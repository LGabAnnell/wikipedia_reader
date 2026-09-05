// wikipedia_featured_client.cpp
#include "wikipedia_featured_client.h"
#include <QUrl>

WikipediaFeaturedClient::WikipediaFeaturedClient(QObject *parent)
    : QObject(parent), networkManager(new QNetworkAccessManager(this)) {}

WikipediaFeaturedClient::~WikipediaFeaturedClient() = default;

void WikipediaFeaturedClient::getFeaturedArticleOfTheDay() {
    QDate currentDate = QDate::currentDate();
    QString dateString = currentDate.toString("yyyy/MM/dd");

    QUrl url(QString("https://api.wikimedia.org/feed/v1/wikipedia/en/featured/%1").arg(dateString));

    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        this->onFeaturedArticleReply(reply);
        reply->deleteLater();
    });
}

void WikipediaFeaturedClient::onFeaturedArticleReply(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QString title, extract;
        int pageid;
        parseFeaturedArticle(reply->readAll(), title, extract, pageid);
        emit featuredArticleReceived(title, extract, pageid);
    } else {
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
