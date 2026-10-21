// wikipedia_search_client.cpp
#include "wikipedia_search_client.h"
#include "wikipedia_network_access_manager.h"
#include <QJsonParseError>
#include <QUrl>
#include <QUrlQuery>

WikipediaSearchClient::WikipediaSearchClient(QObject *parent)
    : QObject(parent), networkManager(WikipediaNetwork::createNetworkAccessManager(this)) {
    setLanguage("en");
}

WikipediaSearchClient::~WikipediaSearchClient() = default;

void WikipediaSearchClient::setLanguage(const QString &langCode) {
    m_requests.setLanguage(langCode);
    baseUrl = QString("https://%1.wikipedia.org/w/api.php").arg(langCode);
}

void WikipediaSearchClient::cancelPendingRequests() { m_requests.cancelPendingRequests(); }

void WikipediaSearchClient::search(const QString &query, int limit) {
    QUrl url(baseUrl);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("action", "query");
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("list", "search");
    urlQuery.addQueryItem("srsearch", query);
    urlQuery.addQueryItem("srlimit", QString::number(limit));
    url.setQuery(urlQuery);

    const auto context = m_requests.context();
    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    m_requests.track(reply, context);
    connect(reply, &QNetworkReply::finished, this, [this, reply, context]() { this->onSearchReply(reply, context); });
}

void WikipediaSearchClient::onSearchReply(QNetworkReply *reply, const WikipediaRequestContext &context) {
    if (m_requests.isCurrent(context) && reply->error() == QNetworkReply::NoError) {
        const QByteArray responseData = reply->readAll();
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(responseData, &parseError);
        const QJsonObject root = document.object();
        const QJsonValue apiError = root.value("error");
        if (apiError.isObject()) {
            const QString info = apiError.toObject().value("info").toString().trimmed();
            emit errorOccurred(info.isEmpty() ? tr("Wikipedia API returned an error.") : info);
        } else if (parseError.error != QJsonParseError::NoError || !document.isObject() ||
                   !root.value("query").isObject() ||
                   !root.value("query").toObject().value("search").isArray()) {
            emit errorOccurred(tr("Invalid search response."));
        } else {
            emit searchCompleted(parseSearchResults(responseData));
        }
    } else if (m_requests.isCurrent(context) && reply->error() != QNetworkReply::OperationCanceledError) {
        emit errorOccurred(reply->errorString());
    }
    reply->deleteLater();
}

QVector<search_result> WikipediaSearchClient::parseSearchResults(const QByteArray &responseData) {
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    QJsonObject jsonObj = jsonDoc.object();
    QJsonArray searchResults = jsonObj["query"].toObject()["search"].toArray();

    QVector<search_result> results;
    for (const QJsonValue &result : searchResults) {
        results.push_back({result["title"].toString(), result["snippet"].toString(), result["pageid"].toInt()});
    }
    return results;
}
