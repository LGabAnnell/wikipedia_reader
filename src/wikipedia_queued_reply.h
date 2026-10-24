#ifndef WIKIPEDIA_QUEUED_REPLY_H
#define WIKIPEDIA_QUEUED_REPLY_H

#include "wikipedia_request_context.h"
#include <QDateTime>
#include <QNetworkReply>
#include <QPointer>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <functional>
#include <memory>
#include <QNetworkAccessManager>
#include <cstring>
#include <QHash>

namespace WikipediaNetwork {

inline QString requestOrigin(const QUrl &url) {
    const int defaultPort = url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0 ? 443 : 80;
    const int port = url.port(defaultPort);
    return url.scheme().toCaseFolded() + QStringLiteral("://") + url.host().toCaseFolded()
        + QLatin1Char(':') + QString::number(port);
}

/** @brief Shared request budget for API clients and the QML image thread. */
class DownloadQueue {
  public:
    bool acquire(const QString &origin) {
        QMutexLocker lock(&m_mutex);
        const qint64 resumeAt = m_resumeAt.value(origin, 0);
        if (m_active >= 3 || resumeAt > wikipediaMonotonicNowMs())
            return false;
        ++m_active;
        return true;
    }
    void release() {
        QMutexLocker lock(&m_mutex);
        --m_active;
    }
    void pause(const QString &origin, qint64 milliseconds) {
        QMutexLocker lock(&m_mutex);
        m_resumeAt[origin] = qMax(m_resumeAt.value(origin, 0), wikipediaMonotonicNowMs() + milliseconds);
    }
  private:
    QMutex m_mutex;
    int m_active = 0;
    QHash<QString, qint64> m_resumeAt;
};

/** @brief A cancellable GET reply with bounded concurrency and rate-limit retries. */
class QueuedReply final : public QNetworkReply {
  public:
    QueuedReply(const QNetworkRequest &request, QObject *parent,
                std::shared_ptr<DownloadQueue> queue, std::function<QNetworkReply *()> start)
        : QNetworkReply(parent), m_queue(std::move(queue)), m_start(std::move(start)),
          m_deadline(wikipediaMonotonicNowMs() + 45'000) {
        setRequest(request);
        setUrl(request.url());
        setOperation(QNetworkAccessManager::GetOperation);
        open(QIODevice::ReadOnly);
        const qint64 requestedDeadline = request.attribute(static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 1)).toLongLong();
        if (requestedDeadline > 0)
            m_deadline = requestedDeadline;
        const int remainingMs = int(qMax(qint64(1), m_deadline - wikipediaMonotonicNowMs()));
        m_deadlineTimer.setSingleShot(true);
        connect(&m_deadlineTimer, &QTimer::timeout, this, [this] { fail(QNetworkReply::TimeoutError, tr("Request deadline exceeded")); });
        m_deadlineTimer.start(remainingMs);
        QTimer::singleShot(0, this, [this] { dispatch(); });
    }
    ~QueuedReply() override {
        if (m_reply) {
            disconnect(m_reply, nullptr, this, nullptr);
            m_reply->abort();
            m_reply->deleteLater();
            m_queue->release();
        }
    }
    void abort() override {
        if (isFinished()) return;
        m_deadlineTimer.stop();
        if (m_reply) {
            disconnect(m_reply, nullptr, this, nullptr);
            m_reply->abort();
            m_reply->deleteLater();
            m_reply = nullptr;
            m_queue->release();
        }
        setError(OperationCanceledError, tr("Request canceled"));
        setFinished(true);
        emit errorOccurred(error());
        emit finished();
    }
    qint64 bytesAvailable() const override {
        return m_data.size() - m_offset + QNetworkReply::bytesAvailable();
    }
  protected:
    qint64 readData(char *data, qint64 maxSize) override {
        const qint64 count = qMin(maxSize, qint64(m_data.size()) - m_offset);
        if (count <= 0) return isFinished() ? -1 : 0;
        std::memcpy(data, m_data.constData() + m_offset, size_t(count));
        m_offset += count;
        return count;
    }
  private:
    void dispatch() {
        if (isFinished()) return;
        if (wikipediaMonotonicNowMs() >= m_deadline) {
            fail(QNetworkReply::TimeoutError, tr("Request deadline exceeded"));
            return;
        }
        if (!m_queue->acquire(requestOrigin(request().url()))) {
            QTimer::singleShot(100, this, [this] { dispatch(); });
            return;
        }
        m_reply = m_start();
        connect(m_reply, &QNetworkReply::readyRead, this, [this] {
            if (m_reply && !isFinished()) {
                const QByteArray bytes = m_reply->readAll();
                const qint64 maxBytes = request().attribute(static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 2)).toLongLong();
                if (maxBytes > 0 && m_data.size() + bytes.size() > maxBytes) {
                    fail(QNetworkReply::ContentOperationNotPermittedError, tr("Response exceeded size limit"));
                    return;
                }
                m_data.append(bytes);
            }
        });
        connect(m_reply, &QNetworkReply::finished, this, [this] {
            auto *reply = m_reply.data();
            m_reply = nullptr;
            m_queue->release();
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status == 429 || status == 503) {
                const QByteArray header = reply->rawHeader("Retry-After");
                bool numeric = false;
                qint64 seconds = header.toLongLong(&numeric);
                if (!numeric) {
                    const auto date = QDateTime::fromString(QString::fromLatin1(header), Qt::RFC2822Date);
                    seconds = date.isValid() ? QDateTime::currentDateTimeUtc().secsTo(date) : 5;
                }
                m_queue->pause(requestOrigin(request().url()), qBound(qint64(1), seconds, qint64(86'400)) * 1000);
                if (++m_retries <= 2) {
                    m_data.clear();
                    m_offset = 0;
                    reply->deleteLater();
                    dispatch();
                    return;
                }
            }
            setUrl(reply->url());
            for (const auto &header : reply->rawHeaderPairs())
                setRawHeader(header.first, header.second);
            for (auto key : {QNetworkRequest::HttpStatusCodeAttribute,
                             QNetworkRequest::HttpReasonPhraseAttribute,
                             QNetworkRequest::RedirectionTargetAttribute,
                             QNetworkRequest::SourceIsFromCacheAttribute})
                setAttribute(key, reply->attribute(key));
            const QByteArray remainder = reply->readAll();
            const qint64 maxBytes = request().attribute(static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 2)).toLongLong();
            if (maxBytes > 0 && m_data.size() + remainder.size() > maxBytes) {
                reply->deleteLater();
                fail(QNetworkReply::ContentOperationNotPermittedError, tr("Response exceeded size limit"));
                return;
            }
            m_data.append(remainder);
            if (reply->error() != NoError)
                setError(reply->error(), reply->errorString());
            reply->deleteLater();
            setFinished(true);
            m_deadlineTimer.stop();
            emit metaDataChanged();
            if (!m_data.isEmpty()) emit readyRead();
            if (error() != NoError) emit errorOccurred(error());
            emit finished();
        });
    }
    void fail(QNetworkReply::NetworkError code, const QString &message) {
        if (isFinished()) return;
        if (m_reply) {
            disconnect(m_reply, nullptr, this, nullptr);
            m_reply->abort();
            m_reply->deleteLater();
            m_reply = nullptr;
            m_queue->release();
        }
        m_deadlineTimer.stop();
        setError(code, message);
        setFinished(true);
        emit errorOccurred(code);
        if (!m_data.isEmpty()) emit readyRead();
        emit finished();
    }
    std::shared_ptr<DownloadQueue> m_queue;
    std::function<QNetworkReply *()> m_start;
    QPointer<QNetworkReply> m_reply;
    QByteArray m_data;
    qint64 m_offset = 0;
    int m_retries = 0;
    QTimer m_deadlineTimer;
    qint64 m_deadline;
};
} // namespace WikipediaNetwork
#endif
