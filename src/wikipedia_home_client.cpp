// wikipedia_home_client.cpp
#include "wikipedia_home_client.h"
#include <QUrlQuery>
#include <QUrl>

WikipediaHomeClient::WikipediaHomeClient(QObject *parent) : QObject(parent), networkManager(new QNetworkAccessManager(this)) {
}

WikipediaHomeClient::~WikipediaHomeClient() = default;

void WikipediaHomeClient::getNewsItems() {
    QDate currentDate = QDate::currentDate();
    QString dateString = currentDate.toString("yyyy/MM/dd");
    QUrl url(QString("https://api.wikimedia.org/feed/v1/wikipedia/en/featured/%1").arg(dateString));
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onNewsItemsReply(reply); });
}

void WikipediaHomeClient::getOnThisDayEvents(int month, int day) {
    QDate today = QDate::currentDate();
    QUrl url(QString("https://api.wikimedia.org/feed/v1/wikipedia/en/onthisday/all/%1/%2")
                  .arg(today.month(), 2, 10, QLatin1Char('0'))
                  .arg(today.day(), 2, 10, QLatin1Char('0')));
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onOnThisDayEventsReply(reply); });
}

void WikipediaHomeClient::getDidYouKnowItems() {
    QUrl url("https://en.wikipedia.org/api/rest_v1/page/random/title");
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onRandomArticleTitleReply(reply); });
}

void WikipediaHomeClient::onNewsItemsReply(QNetworkReply *reply) {
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

void WikipediaHomeClient::onOnThisDayEventsReply(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(tr("Error fetching on-this-day events: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();

    QVector<on_this_day_event> onThisDayEvents;

    if (root.contains("selected") && root["selected"].isArray()) {
        QJsonArray events = root["selected"].toArray();
        for (const auto &event : events) {
            QJsonObject eventObj = event.toObject();
            on_this_day_event otd;
            otd.year = eventObj["year"].toInt();
            otd.event = eventObj["text"].toString();

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
                            } else {
                                qWarning() << "'page' key not found in desktop URLs";
                            }
                        } else {
                            qWarning() << "'desktop' key not found in content URLs";
                        }
                    } else {
                        qWarning() << "'content_urls' key not found in first page";
                    }
                } else {
                    qWarning() << "Pages array is empty";
                }
            } else {
                qWarning() << "'pages' key not found in event or is not an array";
            }
            onThisDayEvents.append(otd);
        }
    } else {
        qWarning() << "No 'selected' events found in on-this-day API response";
        qWarning() << "API Response:" << QString(data);
    }

    emit onThisDayEventsReceived(onThisDayEvents);
    reply->deleteLater();
}

void WikipediaHomeClient::onRandomArticleTitleReply(QNetworkReply *reply) {
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

void WikipediaHomeClient::fetchArticleContent(const QString &title) {
    QUrl url(QString("https://en.wikipedia.org/api/rest_v1/page/summary/%1").arg(title));
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, title]() { onArticleContentReply(reply, title); });
}

void WikipediaHomeClient::onArticleContentReply(QNetworkReply *reply, const QString &title) {
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
