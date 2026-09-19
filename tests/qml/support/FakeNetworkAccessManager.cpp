#include "FakeNetworkAccessManager.h"

#include <QCoreApplication>
#include <QFile>
#include <QMutexLocker>
#include <QUrlQuery>
#include <algorithm>

#ifndef QML_TEST_FIXTURE_DIR
#define QML_TEST_FIXTURE_DIR "."
#endif

FakeNetworkAccessManager::FakeNetworkAccessManager(NetworkFixtureController *controller, QObject *parent)
    : QNetworkAccessManager(parent), m_controller(controller) {}

QNetworkReply *FakeNetworkAccessManager::createRequest(Operation operation, const QNetworkRequest &request,
                                                        QIODevice *outgoingData) {
    Q_UNUSED(outgoingData);
    return m_controller->handleRequest(operation, request, this);
}

NetworkFixtureController::NetworkFixtureController(QObject *parent) : QObject(parent) {
    addFixtureFile(QStringLiteral("GET"),
                   QUrl(QStringLiteral("https://en.wikipedia.org/w/api.php?action=query&format=json&list=search&srsearch=fixture&srlimit=10")),
                   QStringLiteral("search-results.json"), QStringLiteral("application/json"));
    addFixtureFile(QStringLiteral("GET"), QUrl(QStringLiteral("https://en.wikipedia.org/api/rest_v1/page/html/Fixture_article")),
                   QStringLiteral("article.html"), QStringLiteral("text/html; charset=utf-8"));
    addFixtureFile(QStringLiteral("GET"), QUrl(QStringLiteral("https://upload.wikimedia.org/wikipedia/commons/fixture.png")),
                   QStringLiteral("fixture.png"), QStringLiteral("image/png"));
}

QNetworkAccessManager *NetworkFixtureController::create(QObject *parent) {
    return new FakeNetworkAccessManager(this, parent);
}

int NetworkFixtureController::requestCount() const {
    QMutexLocker locker(&m_mutex);
    return m_requests.size();
}

QVariantList NetworkFixtureController::requests() const {
    QMutexLocker locker(&m_mutex);
    return m_requests;
}

void NetworkFixtureController::addFixture(const QString &method, const QUrl &url, const QByteArray &body,
                                          const QString &contentType, int statusCode, int networkError,
                                          const QString &errorString) {
    FakeNetworkResponse response;
    response.body = body;
    response.contentType = contentType.toUtf8();
    response.statusCode = statusCode;
    response.networkError = static_cast<QNetworkReply::NetworkError>(networkError);
    response.errorString = errorString;
    QMutexLocker locker(&m_mutex);
    m_fixtures.insert(requestKey(method, url), response);
}

void NetworkFixtureController::clearRequests() {
    {
        QMutexLocker locker(&m_mutex);
        if (m_requests.isEmpty()) {
            return;
        }
        m_requests.clear();
    }
    emit requestsChanged();
}

QString NetworkFixtureController::requestKey(const QString &method, const QUrl &url) {
    auto queryItems = QUrlQuery(url).queryItems(QUrl::FullyDecoded);
    std::sort(queryItems.begin(), queryItems.end(), [](const auto &left, const auto &right) {
        return left.first == right.first ? left.second < right.second : left.first < right.first;
    });

    QStringList queryParts;
    queryParts.reserve(queryItems.size());
    for (const auto &item : queryItems) {
        queryParts.append(QString::fromLatin1(QUrl::toPercentEncoding(item.first)) + QLatin1Char('=') +
                          QString::fromLatin1(QUrl::toPercentEncoding(item.second)));
    }

    return method.trimmed().toUpper() + QLatin1Char(' ') + url.host().toLower() + url.path() + QLatin1Char('?') +
           queryParts.join(QLatin1Char('&'));
}

QByteArray NetworkFixtureController::methodForOperation(QNetworkAccessManager::Operation operation,
                                                         const QNetworkRequest &request) {
    switch (operation) {
    case QNetworkAccessManager::GetOperation:
        return "GET";
    case QNetworkAccessManager::HeadOperation:
        return "HEAD";
    case QNetworkAccessManager::PostOperation:
        return "POST";
    case QNetworkAccessManager::PutOperation:
        return "PUT";
    case QNetworkAccessManager::DeleteOperation:
        return "DELETE";
    case QNetworkAccessManager::CustomOperation:
        return request.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray().toUpper();
    }
    return "UNKNOWN";
}

FakeNetworkReply *NetworkFixtureController::handleRequest(QNetworkAccessManager::Operation operation,
                                                           const QNetworkRequest &request, QObject *parent) {
    const QByteArray method = methodForOperation(operation, request);
    const QString key = requestKey(QString::fromLatin1(method), request.url());
    FakeNetworkResponse response;
    bool matched = false;
    {
        QMutexLocker locker(&m_mutex);
        const auto fixture = m_fixtures.constFind(key);
        if (fixture != m_fixtures.cend()) {
            response = fixture.value();
            matched = true;
        }

        QVariantMap record;
        record.insert(QStringLiteral("method"), QString::fromLatin1(method));
        record.insert(QStringLiteral("host"), request.url().host());
        record.insert(QStringLiteral("path"), request.url().path());
        record.insert(QStringLiteral("query"), QUrlQuery(request.url()).query(QUrl::FullyDecoded));
        record.insert(QStringLiteral("url"), request.url().toString());
        record.insert(QStringLiteral("matched"), matched);
        m_requests.append(record);
    }
    emit requestsChanged();

    if (!matched) {
        const QString description = QStringLiteral("Unexpected network request: %1 %2")
                                        .arg(QString::fromLatin1(method), request.url().toString());
        emit unexpectedRequest(description);

        FakeNetworkResponse rejected;
        rejected.contentType = "text/plain";
        rejected.networkError = QNetworkReply::ContentNotFoundError;
        rejected.errorString = description;
        return new FakeNetworkReply(request, operation, rejected, parent);
    }

    return new FakeNetworkReply(request, operation, response, parent);
}

void NetworkFixtureController::addFixtureFile(const QString &method, const QUrl &url, const QString &fixtureFile,
                                              const QString &contentType) {
    QFile file(QStringLiteral(QML_TEST_FIXTURE_DIR) + QLatin1Char('/') + fixtureFile);
    if (!file.open(QIODevice::ReadOnly)) {
        qFatal("Could not open QML network fixture: %s", qPrintable(file.fileName()));
    }
    addFixture(method, url, file.readAll(), contentType);
}
