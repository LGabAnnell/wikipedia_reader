// wikipedia_home_client.cpp
#include "wikipedia_home_client.h"
#include "wikipedia_network_access_manager.h"
#include <QTextDocumentFragment>
#include <QUrl>
#include <QUrlQuery>

WikipediaHomeClient::WikipediaHomeClient(QObject *parent)
    : QObject(parent), networkManager(WikipediaNetwork::createNetworkAccessManager(this)) {}

WikipediaHomeClient::~WikipediaHomeClient() = default;

void WikipediaHomeClient::setLanguage(const QString &langCode) { m_requests.setLanguage(langCode); }

void WikipediaHomeClient::cancelPendingRequests() { m_requests.cancelPendingRequests(); }

void WikipediaHomeClient::getNewsItems() { fetchNewsItems(QDate::currentDate(), m_requests.context(), 0, {}); }

void WikipediaHomeClient::fetchNewsItems(const QDate &date, const WikipediaRequestContext &context, int daysBack,
                                         const QVector<news_item> &fallbackItems, bool alternate) {
    if (!m_requests.isCurrent(context))
        return;
    const QString dateString = date.toString("yyyy/MM/dd");
    const QString endpoint = alternate
        ? "https://%1.wikipedia.org/api/rest_v1/feed/featured/%2"
        : "https://api.wikimedia.org/feed/v1/wikipedia/%1/featured/%2";
    const QUrl url(endpoint.arg(context.language, dateString));
    QNetworkRequest request(url);
    if (alternate)
        request.setRawHeader("Cache-Control", "no-cache");
    QNetworkReply *reply = networkManager->get(request);
    m_requests.track(reply, context);
    connect(reply, &QNetworkReply::finished, this, [this, reply, context, date, daysBack, fallbackItems, alternate]() {
        onNewsItemsReply(reply, context, date, daysBack, fallbackItems, alternate);
    });
}

void WikipediaHomeClient::getOnThisDayEvents(int month, int day) {
    const auto context = m_requests.context();
    QUrl url(QString("https://api.wikimedia.org/feed/v1/wikipedia/%1/onthisday/all/%2/%3")
                 .arg(context.language)
                 .arg(month, 2, 10, QLatin1Char('0'))
                 .arg(day, 2, 10, QLatin1Char('0')));
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    m_requests.track(reply, context);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, context]() { onOnThisDayEventsReply(reply, context); });
}

void WikipediaHomeClient::getDidYouKnowItems() {
    const auto context = m_requests.context();
    QUrl url(QString("https://%1.wikipedia.org/api/rest_v1/page/random/title").arg(context.language));
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    m_requests.track(reply, context);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, context]() { onRandomArticleTitleReply(reply, context); });
}

void WikipediaHomeClient::onNewsItemsReply(QNetworkReply *reply, const WikipediaRequestContext &context,
                                           const QDate &date, int daysBack, QVector<news_item> fallbackItems,
                                           bool alternate) {
    if (!m_requests.isCurrent(context)) {
        reply->deleteLater();
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Error fetching news:" << reply->errorString();
        if (daysBack == 0) {
            const QString error = tr("Error fetching news: %1").arg(reply->errorString());
            emit sectionError(News, error);
            emit errorOccurred(error);
            reply->deleteLater();
            return;
        }
    } else {
        const QByteArray data = reply->readAll();
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(data, &parseError);
        const QJsonObject root = document.object();
        if (!alternate && parseError.error == QJsonParseError::NoError && root.size() == 1 &&
            root.contains("onthisday")) {
            // A partial feed is not an empty news section. Retry the equivalent
            // route once before reporting a failure or checking an older date.
            reply->deleteLater();
            fetchNewsItems(date, context, daysBack, fallbackItems, true);
            return;
        }
        const bool validShape =
            document.isObject() && !root.contains("error") &&
            ((root.value("mostread").isObject() && root.value("mostread").toObject().value("articles").isArray()) ||
             root.value("news").isArray());
        bool validItems = validShape;
        if (validItems) {
            const QJsonArray mostread = root.value("mostread").toObject().value("articles").toArray();
            if (!mostread.isEmpty()) {
                for (const auto &value : mostread) {
                    const auto article = value.toObject();
                    const QString title = article.value("titles").toObject().value("normalized").toString(
                        article.value("title").toString());
                    if (!value.isObject() || title.trimmed().isEmpty() ||
                        article.value("pageid").toInt() <= 0) {
                        validItems = false;
                        break;
                    }
                }
            } else {
                for (const auto &value : root.value("news").toArray()) {
                    const auto entry = value.toObject();
                    const QJsonArray links = entry.value("links").toArray();
                    if (!value.isObject() || links.isEmpty() || !links.first().isObject() ||
                        links.first().toObject().value("title").toString().trimmed().isEmpty() ||
                        links.first().toObject().value("pageid").toInt() <= 0) {
                        validItems = false;
                        break;
                    }
                }
            }
        }
        if (parseError.error != QJsonParseError::NoError || !validItems) {
            reply->deleteLater();
            if (daysBack == 0) {
                const QString error = tr("Invalid response fetching news");
                emit sectionError(News, error);
                emit errorOccurred(error);
                return;
            }
            if (daysBack < 2)
                fetchNewsItems(date.addDays(-1), context, daysBack + 1, fallbackItems);
            else
                emit newsItemsReceived(fallbackItems);
            return;
        }
        if (!root["mostread"].toObject()["articles"].toArray().isEmpty()) {
            emit newsItemsReceived(parseNewsItems(data));
            reply->deleteLater();
            return;
        }
        if (daysBack == 0)
            fallbackItems = parseNewsItems(data);
    }

    reply->deleteLater();
    // Most-read statistics can lag behind the rest of the daily feed. Look back
    // at most two days, retaining today's news if none of those feeds has them.
    if (daysBack < 2)
        fetchNewsItems(date.addDays(-1), context, daysBack + 1, fallbackItems);
    else
        emit newsItemsReceived(fallbackItems);
}

QVector<news_item> WikipediaHomeClient::parseNewsItems(const QByteArray &responseData) {
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    QJsonObject root = doc.object();

    QVector<news_item> newsItems;
    // Preserve the full article list; use news for editions/dates without mostread.
    const QJsonArray mostread = root["mostread"].toObject()["articles"].toArray();
    const bool useNews = mostread.isEmpty();
    const QJsonArray items = useNews ? root["news"].toArray() : mostread;
    for (const auto &item : items) {
        const QJsonObject entry = item.toObject();
        QJsonObject article = entry;
        if (useNews) {
            // A news story links to one or more article summaries.
            const QJsonArray links = entry["links"].toArray();
            if (links.isEmpty())
                continue;
            article = links.first().toObject();
        }
        news_item ni;
        ni.title = article["titles"].toObject()["normalized"].toString(article["title"].toString());
        ni.description = useNews ? QTextDocumentFragment::fromHtml(entry["story"].toString()).toPlainText()
                                 : article["extract"].toString();
        ni.url = article["content_urls"].toObject()["desktop"].toObject()["page"].toString();
        ni.pageid = article["pageid"].toInt();
        ni.imageUrl = article["thumbnail"].toObject()["source"].toString();
        newsItems.append(ni);
    }

    return newsItems;
}

void WikipediaHomeClient::onOnThisDayEventsReply(QNetworkReply *reply, const WikipediaRequestContext &context) {
    if (!m_requests.isCurrent(context)) {
        reply->deleteLater();
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        const QString error = tr("Error fetching on-this-day events: %1").arg(reply->errorString());
        emit sectionError(OnThisDay, error);
        emit errorOccurred(error);
        reply->deleteLater();
        return;
    }

    const QByteArray data = reply->readAll();
    const auto doc = QJsonDocument::fromJson(data);
    bool validEvents = doc.isObject() && !doc.object().contains("error") && doc.object().value("selected").isArray();
    if (validEvents) {
        for (const auto &value : doc.object().value("selected").toArray()) {
            const auto event = value.toObject();
            if (!value.isObject() || event.value("year").toInt() == 0 ||
                event.value("text").toString().trimmed().isEmpty()) {
                validEvents = false;
                break;
            }
        }
    }
    if (!validEvents) {
        const QString error = tr("Invalid response fetching on-this-day events");
        emit sectionError(OnThisDay, error);
        emit errorOccurred(error);
    } else
        emit onThisDayEventsReceived(parseOnThisDayEvents(data));
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
    if (!m_requests.isCurrent(context)) {
        reply->deleteLater();
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        const QString error = tr("Error fetching random article: %1").arg(reply->errorString());
        emit sectionError(DidYouKnow, error);
        emit errorOccurred(error);
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();

    if (!doc.isObject() || root.contains("error") || !root.contains("items") || !root["items"].isArray()) {
        emit sectionError(DidYouKnow, tr("Invalid response fetching random article"));
        emit errorOccurred(tr("Invalid response fetching random article"));
    } else {
        const QJsonArray items = root["items"].toArray();
        if (items.isEmpty())
            emit didYouKnowItemsReceived({});
        else {
            const QString title = items[0].toObject()["title"].toString();
            if (title.trimmed().isEmpty()) {
                emit sectionError(DidYouKnow, tr("Invalid random article title"));
                emit errorOccurred(tr("Invalid random article title"));
            } else
                fetchArticleContent(title, context);
        }
    }
    reply->deleteLater();
}

void WikipediaHomeClient::fetchArticleContent(const QString &title, const WikipediaRequestContext &context) {
    if (!m_requests.isCurrent(context))
        return;
    QUrl url(QString("https://%1.wikipedia.org/api/rest_v1/page/summary/%2").arg(context.language, title));
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    m_requests.track(reply, context);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, title, context]() { onArticleContentReply(reply, title, context); });
}

void WikipediaHomeClient::onArticleContentReply(QNetworkReply *reply, const QString &title,
                                                const WikipediaRequestContext &context) {
    Q_UNUSED(title);
    if (!m_requests.isCurrent(context)) {
        reply->deleteLater();
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        const QString error = tr("Error fetching article content: %1").arg(reply->errorString());
        emit sectionError(DidYouKnow, error);
        emit errorOccurred(error);
        reply->deleteLater();
        return;
    }

    const QByteArray data = reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    const auto item = parseArticleContent(data);
    const auto summary = doc.object();
    const QString pageUrl =
        summary.value("content_urls").toObject().value("desktop").toObject().value("page").toString();
    if (!doc.isObject() || summary.contains("error") || item.text.trimmed().isEmpty() || item.pageid <= 0 ||
        pageUrl.isEmpty()) {
        const QString error = tr("Invalid response fetching article content");
        emit sectionError(DidYouKnow, error);
        emit errorOccurred(error);
    } else {
        emit didYouKnowItemsReceived({item});
    }
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
