// wikipedia_home_client.cpp
#include "wikipedia_home_client.h"
#include "wikipedia_network_access_manager.h"
#include <QUrl>
#include <QUrlQuery>

WikipediaHomeClient::WikipediaHomeClient(QObject *parent)
    : QObject(parent), networkManager(WikipediaNetwork::createNetworkAccessManager(this)) {}

WikipediaHomeClient::~WikipediaHomeClient() = default;

void WikipediaHomeClient::setLanguage(const QString &langCode) {
    m_requests.setLanguage(langCode);
}

void WikipediaHomeClient::cancelPendingRequests() { m_requests.cancelPendingRequests(); }

void WikipediaHomeClient::getNewsItems() {
    QDate currentDate = QDate::currentDate();
    QString dateString = currentDate.toString("yyyy/MM/dd");
    const auto context = m_requests.context();
    QUrl url(QString("https://api.wikimedia.org/feed/v1/wikipedia/%1/featured/%2").arg(context.language, dateString));
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    m_requests.track(reply, context);
    connect(reply, &QNetworkReply::finished, this, [this, reply, context]() { onNewsItemsReply(reply, context); });
}

void WikipediaHomeClient::getOnThisDayEvents(int month, int day) {
    QDate today = QDate::currentDate();
    const auto context = m_requests.context();
    QUrl url(QString("https://api.wikimedia.org/feed/v1/wikipedia/%1/onthisday/all/%2/%3")
                 .arg(context.language)
                 .arg(today.month(), 2, 10, QLatin1Char('0'))
                 .arg(today.day(), 2, 10, QLatin1Char('0')));
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    m_requests.track(reply, context);
    connect(reply, &QNetworkReply::finished, this, [this, reply, context]() { onOnThisDayEventsReply(reply, context); });
}

void WikipediaHomeClient::getDidYouKnowItems() {
    const auto context = m_requests.context();
    QUrl url(QString("https://%1.wikipedia.org/api/rest_v1/page/random/title").arg(context.language));
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    m_requests.track(reply, context);
    connect(reply, &QNetworkReply::finished, this, [this, reply, context]() { onRandomArticleTitleReply(reply, context); });
}

void WikipediaHomeClient::onNewsItemsReply(QNetworkReply *reply, const WikipediaRequestContext &context) {
    if (!m_requests.isCurrent(context)) { reply->deleteLater(); return; }
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Error fetching news:" << reply->errorString();
        emit errorOccurred(tr("Error fetching news: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    emit newsItemsReceived(parseNewsItems(reply->readAll()));
    reply->deleteLater();
}

QVector<news_item> WikipediaHomeClient::parseNewsItems(const QByteArray &responseData) {
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
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
            ni.pageid = article["pageid"].toInt();
            ni.imageUrl = article.contains("thumbnail") ? article["thumbnail"].toObject()["source"].toString()
                                                        : "qrc:/images/news_placeholder.jpg";
            newsItems.append(ni);
        }
    } else {
        qWarning() << "mostread field not found in API response";
    }

    return newsItems;
}

void WikipediaHomeClient::onOnThisDayEventsReply(QNetworkReply *reply, const WikipediaRequestContext &context) {
    if (!m_requests.isCurrent(context)) { reply->deleteLater(); return; }
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(tr("Error fetching on-this-day events: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    emit onThisDayEventsReceived(parseOnThisDayEvents(reply->readAll()));
    reply->deleteLater();
}

QVector<on_this_day_event> WikipediaHomeClient::parseOnThisDayEvents(const QByteArray &responseData) {
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    QJsonObject root = doc.object();

    QVector<on_this_day_event> onThisDayEvents;

    if (root.contains("selected") && root["selected"].isArray()) {
        QJsonArray events = root["selected"].toArray();
        for (const auto &event : events) {
            QJsonObject eventObj = event.toObject();
            on_this_day_event otd;
            otd.year = eventObj["year"].toInt();
            otd.event = eventObj["text"].toString();
            otd.pageid = 0; // Default value

            if (eventObj.contains("pages") && eventObj["pages"].isArray()) {
                QJsonArray pages = eventObj["pages"].toArray();
                if (!pages.isEmpty()) {
                    QJsonObject firstPage = pages[0].toObject();
                    if (firstPage.contains("pageid")) {
                        otd.pageid = firstPage["pageid"].toInt();
                    }
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
        qWarning() << "API Response:" << QString(responseData);
    }

    return onThisDayEvents;
}

void WikipediaHomeClient::onRandomArticleTitleReply(QNetworkReply *reply, const WikipediaRequestContext &context) {
    if (!m_requests.isCurrent(context)) { reply->deleteLater(); return; }
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
            fetchArticleContent(title, context);
        }
    }

    reply->deleteLater();
}

void WikipediaHomeClient::fetchArticleContent(const QString &title, const WikipediaRequestContext &context) {
    if (!m_requests.isCurrent(context)) return;
    QUrl url(QString("https://%1.wikipedia.org/api/rest_v1/page/summary/%2").arg(context.language, title));
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    m_requests.track(reply, context);
    connect(reply, &QNetworkReply::finished, this, [this, reply, title, context]() { onArticleContentReply(reply, title, context); });
}

void WikipediaHomeClient::onArticleContentReply(QNetworkReply *reply, const QString &title, const WikipediaRequestContext &context) {
    Q_UNUSED(title);
    if (!m_requests.isCurrent(context)) { reply->deleteLater(); return; }
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(tr("Error fetching article content: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QVector<did_you_know_item> didYouKnowItems;
    didYouKnowItems.append(parseArticleContent(reply->readAll()));

    emit didYouKnowItemsReceived(didYouKnowItems);
    reply->deleteLater();
}

did_you_know_item WikipediaHomeClient::parseArticleContent(const QByteArray &responseData) {
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    QJsonObject root = doc.object();

    did_you_know_item dyk;
    dyk.text = root.contains("extract") ? root["extract"].toString() : "";
    dyk.url =
        root.contains("content_urls") ? root["content_urls"].toObject()["desktop"].toObject()["page"].toString() : "";
    dyk.pageid = root.contains("pageid") ? root["pageid"].toInt() : 0;

    return dyk;
}
