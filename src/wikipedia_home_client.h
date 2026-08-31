// wikipedia_home_client.h
#ifndef WIKIPEDIA_HOME_CLIENT_H
#define WIKIPEDIA_HOME_CLIENT_H

#include "wikipedia_models.h"
#include <QDate>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QVector>

class WikipediaHomeClient : public QObject {
    Q_OBJECT
  public:
    explicit WikipediaHomeClient(QObject *parent = nullptr);
    ~WikipediaHomeClient();

    void getNewsItems();
    void getOnThisDayEvents(int month, int day);
    void getDidYouKnowItems();

  signals:
    void newsItemsReceived(const QVector<news_item> &items);
    void onThisDayEventsReceived(const QVector<on_this_day_event> &events);
    void didYouKnowItemsReceived(const QVector<did_you_know_item> &items);
    void errorOccurred(const QString &error);

  private slots:
    void onNewsItemsReply(QNetworkReply *reply);
    void onOnThisDayEventsReply(QNetworkReply *reply);
    void onRandomArticleTitleReply(QNetworkReply *reply);
    void onArticleContentReply(QNetworkReply *reply, const QString &title);

  private:
    QNetworkAccessManager *networkManager;
    void fetchArticleContent(const QString &title);
};

#endif // WIKIPEDIA_HOME_CLIENT_H
