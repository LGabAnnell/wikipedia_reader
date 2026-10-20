#ifndef WIKIPEDIA_QUEUED_REPLY_H
#define WIKIPEDIA_QUEUED_REPLY_H

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

namespace WikipediaNetwork {

/** @brief Shared request budget for API clients and the QML image thread. */
class DownloadQueue {
  public:
    bool acquire() {
        QMutexLocker lock(&m_mutex);
        if (m_active >= 3 || m_resumeAt > QDateTime::currentMSecsSinceEpoch())
            return false;
        ++m_active;
        return true;
    }
    void release() {
        QMutexLocker lock(&m_mutex);
        --m_active;
    }
    void pause(qint64 milliseconds) {
        QMutexLocker lock(&m_mutex);
        m_resumeAt = qMax(m_resumeAt, QDateTime::currentMSecsSinceEpoch() + milliseconds);
    }
  private:
    QMutex m_mutex;
    int m_active = 0;
    qint64 m_resumeAt = 0;
};

/** @brief A cancellable GET reply with bounded concurrency and rate-limit retries. */
class QueuedReply final : public QNetworkReply {
  public:
    QueuedReply(const QNetworkRequest &request, QObject *parent,
                std::shared_ptr<DownloadQueue> queue, std::function<QNetworkReply *()> start)
        : QNetworkReply(parent), m_queue(std::move(queue)), m_start(std::move(start)) {
        setRequest(request);
        setUrl(request.url());
        setOperation(QNetworkAccessManager::GetOperation);
        open(QIODevice::ReadOnly);
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
        if (!m_queue->acquire()) {
            QTimer::singleShot(100, this, [this] { dispatch(); });
            return;
        }
        m_reply = m_start();
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
                m_queue->pause(qMax(qint64(5), seconds) * 1000);
                if (++m_retries <= 2) {
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
            m_data = reply->readAll();
            if (reply->error() != NoError)
                setError(reply->error(), reply->errorString());
            reply->deleteLater();
            setFinished(true);
            emit metaDataChanged();
            if (!m_data.isEmpty()) emit readyRead();
            if (error() != NoError) emit errorOccurred(error());
            emit finished();
        });
    }
    std::shared_ptr<DownloadQueue> m_queue;
    std::function<QNetworkReply *()> m_start;
    QPointer<QNetworkReply> m_reply;
    QByteArray m_data;
    qint64 m_offset = 0;
    int m_retries = 0;
};
} // namespace WikipediaNetwork
#endif
