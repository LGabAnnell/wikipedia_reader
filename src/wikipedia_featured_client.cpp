// wikipedia_featured_client.cpp
#include "wikipedia_featured_client.h"
#include "wikipedia_network_access_manager.h"
#include <QUrl>

WikipediaFeaturedClient::WikipediaFeaturedClient(QObject *parent)
    : QObject(parent), networkManager(WikipediaNetwork::createNetworkAccessManager(this)) {}

WikipediaFeaturedClient::~WikipediaFeaturedClient() = default;

void WikipediaFeaturedClient::setLanguage(const QString &langCode) {
    m_requests.setLanguage(langCode);
}

void WikipediaFeaturedClient::cancelPendingRequests() { m_requests.cancelPendingRequests(); }

void WikipediaFeaturedClient::getFeaturedArticleOfTheDay() {
    QDate currentDate = QDate::currentDate();
    QString dateString = currentDate.toString("yyyy/MM/dd");

    const auto context = m_requests.context();
    QUrl url(QString("https://api.wikimedia.org/feed/v1/wikipedia/%1/featured/%2").arg(context.language, dateString));

    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    m_requests.track(reply, context);
    connect(reply, &QNetworkReply::finished, this, [this, reply, context]() {
        this->onFeaturedArticleReply(reply, context);
        reply->deleteLater();
    });
}

void WikipediaFeaturedClient::onFeaturedArticleReply(QNetworkReply *reply, const WikipediaRequestContext &context) {
    if (m_requests.isCurrent(context) && reply->error() == QNetworkReply::NoError) {
        QString title, extract;
        int pageid;
        parseFeaturedArticle(reply->readAll(), title, extract, pageid);
        emit featuredArticleReceived(title, extract, pageid);
    } else if (m_requests.isCurrent(context) && reply->error() != QNetworkReply::OperationCanceledError) {
        emit errorOccurred(reply->errorString());
    }
    reply->deleteLater();
}

void WikipediaFeaturedClient::parseFeaturedArticle(const QByteArray &responseData,
                                                    QString &title, QString &extract, int &pageid) {
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    QJsonObject jsonObj = jsonDoc.object();

    extract = jsonObj["tfa"].toObject()["extract"].toString();
    title = jsonObj["tfa"].toObject()["title"].toString();
    pageid = jsonObj["tfa"].toObject()["pageid"].toInt();
}
