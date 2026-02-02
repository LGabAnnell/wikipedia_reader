// wikipedia_featured_client.h
#ifndef WIKIPEDIA_FEATURED_CLIENT_H
#define WIKIPEDIA_FEATURED_CLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QVector>
#include <QDate>
#include "wikipedia_models.h"

class WikipediaFeaturedClient : public QObject
{
    Q_OBJECT
public:
    explicit WikipediaFeaturedClient(QObject *parent = nullptr);
    ~WikipediaFeaturedClient();

    void getFeaturedArticleOfTheDay();

signals:
    void featuredArticleReceived(const QString &title, const QString &extract, const int &pageid);
    void errorOccurred(const QString &error);

private slots:
    void onFeaturedArticleReply(QNetworkReply *reply);

private:
    QNetworkAccessManager *networkManager;
};

#endif // WIKIPEDIA_FEATURED_CLIENT_H
