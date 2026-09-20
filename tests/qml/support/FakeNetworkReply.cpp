#include "FakeNetworkReply.h"

#include <QTimer>
#include <algorithm>

FakeNetworkReply::FakeNetworkReply(const QNetworkRequest &request, QNetworkAccessManager::Operation operation,
                                   const FakeNetworkResponse &response, bool deferred, QObject *parent)
    : QNetworkReply(parent), m_body(response.body), m_networkError(response.networkError),
      m_errorString(response.errorString) {
    setRequest(request);
    setUrl(request.url());
    setOperation(operation);
    setAttribute(QNetworkRequest::HttpStatusCodeAttribute, response.statusCode);
    setHeader(QNetworkRequest::ContentTypeHeader, response.contentType);
    setHeader(QNetworkRequest::ContentLengthHeader, m_body.size());
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);

    if (m_networkError == QNetworkReply::NoError && response.statusCode >= 400) {
        if (response.statusCode == 401) {
            m_networkError = QNetworkReply::AuthenticationRequiredError;
        } else if (response.statusCode == 403) {
            m_networkError = QNetworkReply::ContentAccessDenied;
        } else if (response.statusCode == 404) {
            m_networkError = QNetworkReply::ContentNotFoundError;
        } else {
            m_networkError = QNetworkReply::UnknownContentError;
        }
        if (m_errorString.isEmpty()) {
            m_errorString = QStringLiteral("HTTP %1").arg(response.statusCode);
        }
    }

    if (!deferred) {
        QTimer::singleShot(0, this, &FakeNetworkReply::finishReply);
    }
}

void FakeNetworkReply::complete() { finishReply(); }

qint64 FakeNetworkReply::bytesAvailable() const {
    return (m_body.size() - m_position) + QNetworkReply::bytesAvailable();
}

void FakeNetworkReply::abort() {
    if (m_completed) {
        return;
    }
    m_body.clear();
    m_position = 0;
    m_networkError = QNetworkReply::OperationCanceledError;
    m_errorString = QStringLiteral("Request aborted");
    finishReply();
}

qint64 FakeNetworkReply::readData(char *data, qint64 maxSize) {
    if (maxSize <= 0) {
        return 0;
    }
    if (m_position >= m_body.size()) {
        return -1;
    }

    const qint64 bytesToRead = std::min(maxSize, static_cast<qint64>(m_body.size()) - m_position);
    std::copy_n(m_body.constData() + m_position, bytesToRead, data);
    m_position += bytesToRead;
    return bytesToRead;
}

void FakeNetworkReply::finishReply() {
    if (m_completed) {
        return;
    }
    m_completed = true;
    if (m_networkError != QNetworkReply::NoError) {
        setError(m_networkError, m_errorString);
        emit errorOccurred(m_networkError);
    }
    setFinished(true);
    if (!m_body.isEmpty()) {
        emit readyRead();
    }
    emit finished();
}
