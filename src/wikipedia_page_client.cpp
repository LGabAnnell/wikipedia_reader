// wikipedia_page_client.cpp
#include "wikipedia_page_client.h"
#include "html_processor.h"
#include <QUrlQuery>
#include <QUrl>
#include <QEventLoop>

WikipediaPageClient::WikipediaPageClient(QObject *parent) : QObject(parent), networkManager(new QNetworkAccessManager(this)) {
    baseUrl = "https://en.wikipedia.org/w/api.php";
}

WikipediaPageClient::~WikipediaPageClient() = default;

void WikipediaPageClient::getPage(const QString &title) {
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

void WikipediaPageClient::onPageReply(QNetworkReply *reply, const QString &title) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        QJsonObject jsonObj = jsonDoc.object();
        QJsonObject pages = jsonObj["query"].toObject()["pages"].toObject();

        for (auto it = pages.begin(); it != pages.end(); ++it) {
            if (it.value().toObject()["title"].toString() == title) {
                page page;
                page.title = it.value().toObject()["title"].toString();
                page.extract = it.value().toObject()["extract"].toString();
                page.pageid = it.key().toInt();
                page.imageUrls = QStringList(); // Initialize imageUrls as an empty list
                emit pageReceived(page);
                break;
            }
        }
    } else {
        emit errorOccurred(reply->errorString());
    }
    reply->deleteLater();
}

void WikipediaPageClient::getPageById(int pageid) {
    QUrl url(baseUrl);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("action", "parse");
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("prop", "text");
    urlQuery.addQueryItem("disableeditsection", "true");
    urlQuery.addQueryItem("formatversion", "2");
    urlQuery.addQueryItem("pageid", QString::number(pageid));
    url.setQuery(urlQuery);

    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, pageid]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
            QJsonObject jsonObj = jsonDoc.object();
            QJsonObject pages = jsonObj["parse"].toObject();

            page page;
            page.title = pages["title"].toString();
            page.extract = pages["text"].toString();
            page.extract = HtmlProcessor::processHtml(pages["text"].toString());
            page.pageid = pageid;
            page.imageUrls = QStringList(); // Initialize imageUrls as an empty list
            emit pageReceived(page);
        } else {
            emit errorOccurred(reply->errorString());
        }
        reply->deleteLater();
    });
}

void WikipediaPageClient::getPageWithImages(int pageid) {
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

void WikipediaPageClient::onPageWithImagesReply(QNetworkReply *reply, int pageid) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        QJsonObject jsonObj = jsonDoc.object();
        QJsonObject pages = jsonObj["query"].toObject()["pages"].toObject();

        page page;
        QStringList imageTitles;

        for (auto it = pages.begin(); it != pages.end(); ++it) {
            if (it.key().toInt() == pageid) {
                QJsonObject pageObj = it.value().toObject();
                page.title = pageObj["title"].toString();
                page.extract = pageObj["extract"].toString();
                page.pageid = pageid;
                page.imageUrls = QStringList(); // Initialize imageUrls as an empty list

                // Extract image titles
                if (pageObj.contains("images")) {
                    QJsonArray images = pageObj["images"].toArray();
                    for (const QJsonValue &image : std::as_const(images)) {
                        imageTitles.append(image.toObject().value("title").toString());
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

void WikipediaPageClient::fetchImageUrlsFromTitles(const QStringList &imageTitles, QStringList &imageUrls) {
    if (imageTitles.isEmpty()) {
        return;
    }

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
