// wikipedia_featured_client.cpp
#include "wikipedia_featured_client.h"
#include <QUrlQuery>
#include <QUrl>

WikipediaFeaturedClient::WikipediaFeaturedClient(QObject *parent) : QObject(parent), networkManager(new QNetworkAccessManager(this)) {
}

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
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        QJsonObject jsonObj = jsonDoc.object();

        QString extract = jsonObj["tfa"].toObject()["extract"].toString();
        QString title = jsonObj["tfa"].toObject()["title"].toString();
        int pageid = jsonObj["tfa"].toObject()["pageid"].toInt();
        emit featuredArticleReceived(title, extract, pageid);
    } else {
        emit errorOccurred(reply->errorString());
    }
    reply->deleteLater();
}
