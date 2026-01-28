// WikipediaClient.cpp

#include "wikipedia_client.h"
#include "html_processor.h"
#include <QUrlQuery>
#include <QUrl>
#include <iostream>
#include <QDate>
#include <QEventLoop>
#include <QFile>
#include <tinyxml2.h>



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

void WikipediaClient::getPage(const QString &title) {
    //
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
                page page;
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
    // https://en.wikipedia.org/w/api.php?action=parse&page=Article_Title&format=json&prop=text&contentmodel=wikitext&disableeditsection=true&disabletoc=true&noimages=true
    QUrl url(baseUrl);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("action", "parse");
    urlQuery.addQueryItem("format", "json");
    urlQuery.addQueryItem("prop", "text");
    // urlQuery.addQueryItem("contentmodel", "wikitext");
    urlQuery.addQueryItem("disableeditsection", "true");
    urlQuery.addQueryItem("formatversion", "2");
    urlQuery.addQueryItem("pageid", QString::number(pageid));
    // urlQuery.addQueryItem("explaintext", "1");
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

            emit pageReceived(page);
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

        page page;
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
    // Get the current date in the format YYYY/MM/DD
    QDate currentDate = QDate::currentDate();
    QString dateString = currentDate.toString("yyyy/MM/dd");

    // Construct the URL
    QUrl url(QString("https://api.wikimedia.org/feed/v1/wikipedia/en/featured/%1").arg(dateString));

    // Send the request
    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        this->onFeaturedArticleReply(reply);
        reply->deleteLater();
    });
}

void WikipediaClient::onFeaturedArticleReply(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);

        QJsonObject jsonObj = jsonDoc.object();

        QString extract = jsonObj["tfa"].toObject()["extract"].toString();
        QString title = jsonObj["tfa"].toObject()["title"].toString();
        int pageid = jsonObj["tfa"].toObject()["pageid"].toInt();
        emit featuredArticleReceived(title, extract, pageid); // Emit the featured article with the new structure
    } else {
        emit errorOccurred(reply->errorString());
    }
    reply->deleteLater();
}

// New methods for HomeModel
void WikipediaClient::getNewsItems() {
    // Use Wikipedia's REST API for featured content
    QDate currentDate = QDate::currentDate();
    QString dateString = currentDate.toString("yyyy/MM/dd");
    QUrl url(QString("https://api.wikimedia.org/feed/v1/wikipedia/en/featured/%1").arg(dateString));
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onNewsItemsReply(reply); });
}

void WikipediaClient::getOnThisDayEvents() {
    // Use Wikipedia's REST API for "On This Day"
    QDate today = QDate::currentDate();
    QUrl url(QString("https://api.wikimedia.org/feed/v1/wikipedia/en/onthisday/all/%1/%2")
                  .arg(today.month(), 2, 10, QLatin1Char('0'))
                  .arg(today.day(), 2, 10, QLatin1Char('0')));
    qDebug() << "Fetching On This Day events from URL:" << url.toString();
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onOnThisDayEventsReply(reply); });
}

void WikipediaClient::getDidYouKnowItems() {
    // Fetch a random article and extract its first paragraph
    QUrl url("https://en.wikipedia.org/api/rest_v1/page/random/title");
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onRandomArticleTitleReply(reply); });
}

void WikipediaClient::onNewsItemsReply(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Error fetching news:" << reply->errorString();
        emit errorOccurred(tr("Error fetching news: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();

    QVector<news_item> newsItems;
    if (root.contains("mostread")) {
        QJsonArray items = root["mostread"].toObject()["articles"].toArray();
        for (const auto &item : items) {
            QJsonObject article = item.toObject();
            news_item ni;
            ni.title = article["title"].toString();
            ni.description = article["extract"].toString();
            ni.url = article["content_urls"].toObject()["desktop"].toObject()["page"].toString();
            ni.imageUrl = article.contains("thumbnail") ?
                article["thumbnail"].toObject()["source"].toString() :
                "qrc:/images/news_placeholder.jpg";
            newsItems.append(ni);
        }
    } else {
        qWarning() << "mostread field not found in API response";
    }

    emit newsItemsReceived(newsItems);
    reply->deleteLater();
}

void WikipediaClient::onOnThisDayEventsReply(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(tr("Error fetching on-this-day events: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();

    QVector<on_this_day_event> onThisDayEvents;

    // Check if the response contains the "selected" array which has the on-this-day events
    if (root.contains("selected") && root["selected"].isArray()) {
        QJsonArray events = root["selected"].toArray();
        for (const auto &event : events) {
            QJsonObject eventObj = event.toObject();
            on_this_day_event otd;
            otd.year = eventObj["year"].toInt();
            otd.event = eventObj["text"].toString();

            // Extract the URL from the first page in the "pages" array
            if (eventObj.contains("pages") && eventObj["pages"].isArray()) {
                QJsonArray pages = eventObj["pages"].toArray();
                if (!pages.isEmpty()) {
                    QJsonObject firstPage = pages[0].toObject();
                    if (firstPage.contains("content_urls")) {
                        QJsonObject contentUrls = firstPage["content_urls"].toObject();
                        if (contentUrls.contains("desktop")) {
                            QJsonObject desktopUrls = contentUrls["desktop"].toObject();
                            if (desktopUrls.contains("page")) {
                                otd.url = desktopUrls["page"].toString();
                            }
                        }
                    }
                }
            }
            onThisDayEvents.append(otd);
        }
    } else {
        qWarning() << "No 'selected' events found in on-this-day API response";
        // Log the actual response for debugging
        qWarning() << "API Response:" << QString(data);
    }

    emit onThisDayEventsReceived(onThisDayEvents);
    reply->deleteLater();
}

void WikipediaClient::onRandomArticleTitleReply(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(tr("Error fetching random article: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();

    if (root.contains("items")) {
        QJsonArray items = root["items"].toArray();
        if (!items.isEmpty()) {
            QString title = items[0].toObject()["title"].toString();
            fetchArticleContent(title);
        }
    }

    reply->deleteLater();
}

void WikipediaClient::fetchArticleContent(const QString &title) {
    QUrl url(QString("https://en.wikipedia.org/api/rest_v1/page/summary/%1").arg(title));
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, title]() { onArticleContentReply(reply, title); });
}

void WikipediaClient::onArticleContentReply(QNetworkReply *reply, const QString &title) {
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(tr("Error fetching article content: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();

    QVector<did_you_know_item> didYouKnowItems;
    did_you_know_item dyk;
    dyk.text = root.contains("extract") ? root["extract"].toString() : "";
    dyk.url = root.contains("content_urls") ?
        root["content_urls"].toObject()["desktop"].toObject()["page"].toString() :
        "";
    didYouKnowItems.append(dyk);

    emit didYouKnowItemsReceived(didYouKnowItems);
    reply->deleteLater();
}

