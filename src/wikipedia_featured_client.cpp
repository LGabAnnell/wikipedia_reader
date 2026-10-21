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
    QDate currentDate = QDate::currentDate();
    QString dateString = currentDate.toString("yyyy/MM/dd");

    const auto context = m_requests.context();
    QUrl url(QString("https://%1.wikipedia.org/api/rest_v1/feed/featured/%2").arg(context.language, dateString));

    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    m_requests.track(reply, context);
    connect(reply, &QNetworkReply::finished, this, [this, reply, context]() {
        this->onFeaturedArticleReply(reply, context);
        reply->deleteLater();
    });
}

void WikipediaFeaturedClient::onFeaturedArticleReply(QNetworkReply *reply, const WikipediaRequestContext &context) {
    if (m_requests.isCurrent(context) && reply->error() == QNetworkReply::NoError) {
        const QByteArray data = reply->readAll();
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(data, &parseError);
        const QJsonValue tfaValue = document.object().value("tfa");
        if (parseError.error != QJsonParseError::NoError || !document.isObject() ||
            document.object().contains("error")) {
            emit errorOccurred(tr("Invalid featured article response"));
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
