// WikipediaClient.cpp

#include "wikipedia_client.h"
#include <QUrlQuery>
#include <QUrl>
#include <QDate>
#include <QEventLoop>
#include <iostream>


WikipediaClient::WikipediaClient(QObject *parent) : QObject(parent), networkManager(new QNetworkAccessManager(this)) {
    baseUrl = "https://en.wikipedia.org/w/api.php";
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
            results.push_back({ result["title"].toString(),
                            result["snippet"].toString(),
                            result["pageid"].toInt() });
        }
        qDebug() << "Search completed with" << results.size() << "results";
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
    QUrl url(baseUrl);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("action", "query");
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("prop", "extracts");
    urlQuery.addQueryItem("pageids", QString::number(pageid));
    urlQuery.addQueryItem("explaintext", "1");
    url.setQuery(urlQuery);

    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, pageid]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            QJsonObject jsonObj = jsonDoc.object();
            QJsonObject pages = jsonObj["query"].toObject()["pages"].toObject();

            for (auto it = pages.begin(); it != pages.end(); ++it) {
                if (it.key().toInt() == pageid) {
                    Page page;
                    page.title = it.value().toObject()["title"].toString();
                    page.extract = it.value().toObject()["extract"].toString();
                    page.pageid = pageid;
                    emit pageReceived(page);
                    break;
                }
            }
        } else {
            emit errorOccurred(reply->errorString());
        }
        reply->deleteLater();
    });
}

void WikipediaClient::getPageWithImages(int pageid) {
    QUrl url(baseUrl);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("action", "query");
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("prop", "extracts|images");
    urlQuery.addQueryItem("pageids", QString::number(pageid));
    urlQuery.addQueryItem("explaintext", "1");
    urlQuery.addQueryItem("imlimit", "50");
    url.setQuery(urlQuery);

    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, pageid]() {
        this->onPageWithImagesReply(reply, pageid);
    });
}

void WikipediaClient::onPageWithImagesReply(QNetworkReply *reply, int pageid) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        QJsonObject jsonObj = jsonDoc.object();
        QJsonObject pages = jsonObj["query"].toObject()["pages"].toObject();

        Page page;
        QStringList imageTitles;

        for (auto it = pages.begin(); it != pages.end(); ++it) {
            if (it.key().toInt() == pageid) {
                QJsonObject pageObj = it.value().toObject();
                page.title = pageObj["title"].toString();
                page.extract = pageObj["extract"].toString();
                page.pageid = pageid;

                // Extract image titles
                if (pageObj.contains("images")) {
                    QJsonArray images = pageObj["images"].toArray();
                    for (const QJsonValue &image : images) {
                        imageTitles.append(image.toObject()["title"].toString());
                    }
                }
                break;
            }
        }

        // Fetch image URLs from titles
        fetchImageUrlsFromTitles(imageTitles, page.imageUrls);
        emit pageWithImagesReceived(page);
    } else {
        emit errorOccurred(reply->errorString());
    }
    reply->deleteLater();
}

void WikipediaClient::fetchImageUrlsFromTitles(const QStringList &imageTitles, QStringList &imageUrls) {
    if (imageTitles.isEmpty()) {
        return;
    }

    // For simplicity, we'll use a synchronous approach for image URL fetching
    // In a production environment, you might want to use a more sophisticated approach
    QEventLoop loop;
    QUrl url(baseUrl);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("action", "query");
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("prop", "imageinfo");
    urlQuery.addQueryItem("iiprop", "url");
    urlQuery.addQueryItem("titles", imageTitles.join("|"));
    url.setQuery(urlQuery);

    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        QJsonObject jsonObj = jsonDoc.object();
        QJsonObject pages = jsonObj["query"].toObject()["pages"].toObject();

        for (auto it = pages.begin(); it != pages.end(); ++it) {
            QJsonObject pageObj = it.value().toObject();
            if (pageObj.contains("imageinfo")) {
                QJsonArray imageInfo = pageObj["imageinfo"].toArray();
                if (!imageInfo.isEmpty()) {
                    QString url = imageInfo[0].toObject()["url"].toString();
                    if (!url.isEmpty()) {
                        imageUrls.append(url);
                    }
                }
            }
        }
    }
    reply->deleteLater();
}

void WikipediaClient::getFeaturedArticleOfTheDay() {
    QUrl url(baseUrl);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("action", "featuredfeed");
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("feed", "featured");
    urlQuery.addQueryItem("language", "en");
    urlQuery.addQueryItem("sections", "today");
    url.setQuery(urlQuery);

    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        this->onFeaturedArticleReply(reply);
    });
}

void WikipediaClient::onFeaturedArticleReply(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        QJsonObject jsonObj = jsonDoc.object();

        // Parse the featured article response
        // Note: The actual structure might vary, this is a simplified approach
        if (jsonObj.contains("featured") && jsonObj["featured"].isObject()) {
            QJsonObject featured = jsonObj["featured"].toObject();
            if (featured.contains("today") && featured["today"].isObject()) {
                QJsonObject today = featured["today"].toObject();

                FeaturedArticle article;
                article.date = QDate::currentDate().toString("yyyy-MM-dd");

                // Extract article information - this might need adjustment based on actual API response
                if (today.contains("articles") && today["articles"].isArray()) {
                    QJsonArray articles = today["articles"].toArray();
                    if (!articles.isEmpty()) {
                        QJsonObject articleObj = articles[0].toObject();
                        article.title = articleObj["title"].toString();
                        article.pageid = articleObj["pageid"].toInt();
                        article.extract = articleObj["extract"].toString();

                        // Extract image URLs if available
                        if (articleObj.contains("images") && articleObj["images"].isArray()) {
                            QJsonArray images = articleObj["images"].toArray();
                            for (const QJsonValue &image : images) {
                                article.imageUrls.append(image.toString());
                            }
                        }
                    }
                }

                emit featuredArticleReceived(article);
            }
        }
    } else {
        emit errorOccurred(reply->errorString());
    }
    reply->deleteLater();
}