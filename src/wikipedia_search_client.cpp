// wikipedia_search_client.cpp
#include "wikipedia_search_client.h"
#include <QUrlQuery>
#include <QUrl>

WikipediaSearchClient::WikipediaSearchClient(QObject *parent) : QObject(parent), networkManager(new QNetworkAccessManager(this)) {
    baseUrl = "https://en.wikipedia.org/w/api.php";
}

WikipediaSearchClient::~WikipediaSearchClient() = default;

void WikipediaSearchClient::search(const QString &query, int limit) {
    QUrl url(baseUrl);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("action", "query");
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("list", "search");
    urlQuery.addQueryItem("srsearch", query);
    urlQuery.addQueryItem("srlimit", QString::number(limit));
    url.setQuery(urlQuery);

    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { this->onSearchReply(reply); });
}

void WikipediaSearchClient::onSearchReply(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        QJsonObject jsonObj = jsonDoc.object();
        QJsonArray searchResults = jsonObj["query"].toObject()["search"].toArray();

        QVector<search_result> results;
        for (const QJsonValue &result : searchResults) {
            results.push_back({ result["title"].toString(),
                                result["snippet"].toString(),
                                result["pageid"].toInt() });
        }
        emit searchCompleted(results);
    } else {
        emit errorOccurred(reply->errorString());
    }
    reply->deleteLater();
}
