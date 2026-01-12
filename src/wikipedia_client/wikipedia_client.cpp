// WikipediaClient.cpp

#include "wikipedia_client.h"
#include "components/searchbar/SearchBarModel.h"
#include <QUrlQuery>
#include <QUrl>
#include <iostream>

namespace Wikipedia {

    WikipediaClient::WikipediaClient(QObject *parent) : QObject(parent), networkManager(new QNetworkAccessManager(this)) {
        baseUrl = "https://test.wikipedia.org/w/api.php";
    }

    WikipediaClient::~WikipediaClient() = default; // Default implementation

    void WikipediaClient::search(const QString &query, int limit) {
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

    void WikipediaClient::onSearchReply(QNetworkReply *reply) {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            QJsonObject jsonObj = jsonDoc.object();
            QJsonArray searchResults = jsonObj["query"].toObject()["search"].toArray();

            QVector<SearchResult> results;
            for (const QJsonValue &result : searchResults) {
                results.append({ result["title"].toString(),
                                result["snippet"].toString(),
                                result["pageid"].toInt() });
            }
            emit searchCompleted(results);
        } else {
            emit errorOccurred(reply->errorString());
        }
        reply->deleteLater();
    }

    void WikipediaClient::getPage(const QString &title) {
        QUrl url(baseUrl);
        QUrlQuery urlQuery;
        urlQuery.addQueryItem("action", "query");
        urlQuery.addQueryItem("format", "json");
        urlQuery.addQueryItem("prop", "extracts");
        urlQuery.addQueryItem("titles", title);
        urlQuery.addQueryItem("explaintext", "1");
        url.setQuery(urlQuery);

        QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, this, [this, reply, title]() { this->onPageReply(reply, title); });
    }

    void WikipediaClient::onPageReply(QNetworkReply *reply, const QString &title) {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            QJsonObject jsonObj = jsonDoc.object();
            QJsonObject pages = jsonObj["query"].toObject()["pages"].toObject();

            for (auto it = pages.begin(); it != pages.end(); ++it) {
                if (it.value().toObject()["title"].toString() == title) {
                    Page page;
                    page.title = it.value().toObject()["title"].toString();
                    page.extract = it.value().toObject()["extract"].toString();
                    page.pageid = it.key().toInt();
                    emit pageReceived(page);
                    break;
                }
            }
        } else {
            emit errorOccurred(reply->errorString());
        }
        reply->deleteLater();
    }

    void WikipediaClient::getPageById(int pageid) {
        // Implement similarly to getPage
    }

} // namespace Wikipedia