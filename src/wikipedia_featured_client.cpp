// wikipedia_featured_client.cpp
#include "wikipedia_featured_client.h"
#include "wikipedia_network_access_manager.h"
#include <QJsonParseError>
#include <QUrl>

WikipediaFeaturedClient::WikipediaFeaturedClient(QObject *parent)
    : QObject(parent), networkManager(WikipediaNetwork::createNetworkAccessManager(this)) {}

WikipediaFeaturedClient::~WikipediaFeaturedClient() = default;

void WikipediaFeaturedClient::setLanguage(const QString &langCode) { m_requests.setLanguage(langCode); }

void WikipediaFeaturedClient::cancelPendingRequests() { m_requests.cancelPendingRequests(); }

void WikipediaFeaturedClient::getFeaturedArticleOfTheDay() {
    fetchFeaturedArticle(m_requests.context(), QDate::currentDate(), false);
}

void WikipediaFeaturedClient::fetchFeaturedArticle(const WikipediaRequestContext &context,
                                                   const QDate &date, bool alternate) {
    if (!m_requests.isCurrent(context))
        return;
    const QString endpoint = alternate
        ? "https://%1.wikipedia.org/api/rest_v1/feed/featured/%2"
        : "https://api.wikimedia.org/feed/v1/wikipedia/%1/featured/%2";
    const QUrl url(endpoint.arg(context.language, date.toString("yyyy/MM/dd")));

    QNetworkRequest request(url);
    if (alternate)
        request.setRawHeader("Cache-Control", "no-cache");
    QNetworkReply *reply = networkManager->get(request);
    m_requests.track(reply, context);
    connect(reply, &QNetworkReply::finished, this, [this, reply, context, date, alternate]() {
        onFeaturedArticleReply(reply, context, date, alternate);
    });
}

void WikipediaFeaturedClient::onFeaturedArticleReply(QNetworkReply *reply, const WikipediaRequestContext &context,
                                                     const QDate &date, bool alternate) {
    if (m_requests.isCurrent(context) && reply->error() == QNetworkReply::NoError) {
        const QByteArray data = reply->readAll();
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(data, &parseError);
        const QJsonValue tfaValue = document.object().value("tfa");
        if (parseError.error != QJsonParseError::NoError || !document.isObject() ||
            document.object().contains("error")) {
            emit errorOccurred(tr("Invalid featured article response"));
        } else if (document.object().size() == 1 && document.object().contains("onthisday")) {
            // Both hosts can serve a partial cached feed. Try the equivalent
            // route once, preserving the original edition, date, and generation.
            if (!alternate)
                fetchFeaturedArticle(context, date, true);
            else
                emit errorOccurred(tr("Incomplete featured article response"));
        } else if (tfaValue.isNull() || tfaValue.isUndefined()) {
            emit featuredArticleUnavailable();
        } else {
            QString title, extract, thumbnail;
            int pageid = 0;
            parseFeaturedArticle(data, title, extract, pageid, thumbnail);
            if (title.trimmed().isEmpty() || pageid <= 0)
                emit errorOccurred(tr("Invalid featured article response"));
            else
                emit featuredArticleReceived(title, extract, pageid, thumbnail);
        }
    } else if (m_requests.isCurrent(context) && reply->error() != QNetworkReply::OperationCanceledError) {
        emit errorOccurred(reply->errorString());
    }
    reply->deleteLater();
}

void WikipediaFeaturedClient::parseFeaturedArticle(const QByteArray &responseData, QString &title, QString &extract,
                                                   int &pageid, QString &thumbnailUrl) {
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    QJsonObject jsonObj = jsonDoc.object();

    extract = jsonObj["tfa"].toObject()["extract"].toString();
    title = jsonObj["tfa"].toObject()["title"].toString();
    pageid = jsonObj["tfa"].toObject()["pageid"].toInt();
    thumbnailUrl = jsonObj["tfa"].toObject()["thumbnail"].toObject()["source"].toString();
}
