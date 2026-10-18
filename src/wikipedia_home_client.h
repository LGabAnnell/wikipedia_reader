// wikipedia_home_client.h
#ifndef WIKIPEDIA_HOME_CLIENT_H
#define WIKIPEDIA_HOME_CLIENT_H

#include "wikipedia_models.h"
#include "wikipedia_request_context.h"
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
    enum Section { News, OnThisDay, DidYouKnow };
    Q_ENUM(Section)
    explicit WikipediaHomeClient(QObject *parent = nullptr);
    ~WikipediaHomeClient();

    /** @brief Fetch most-read articles, checking the previous two days if today's feed is incomplete. */
    void getNewsItems();
    void getOnThisDayEvents(int month, int day);
    void getDidYouKnowItems();
    void setLanguage(const QString &langCode);
    void cancelPendingRequests();

    static QVector<news_item> parseNewsItems(const QByteArray &responseData);
    static QVector<on_this_day_event> parseOnThisDayEvents(const QByteArray &responseData);
    static did_you_know_item parseArticleContent(const QByteArray &responseData);

  signals:
    void newsItemsReceived(const QVector<news_item> &items);
    void onThisDayEventsReceived(const QVector<on_this_day_event> &events);
    void didYouKnowItemsReceived(const QVector<did_you_know_item> &items);
    void errorOccurred(const QString &error);
    void sectionError(WikipediaHomeClient::Section section, const QString &error);

  private slots:
    void onNewsItemsReply(QNetworkReply *reply, const WikipediaRequestContext &context, const QDate &date, int daysBack,
                          QVector<news_item> fallbackItems);
    void onOnThisDayEventsReply(QNetworkReply *reply, const WikipediaRequestContext &context);
    void onRandomArticleTitleReply(QNetworkReply *reply, const WikipediaRequestContext &context);
    void onArticleContentReply(QNetworkReply *reply, const QString &title, const WikipediaRequestContext &context);

  private:
    QNetworkAccessManager *networkManager;
    WikipediaRequestTracker m_requests;
    void fetchNewsItems(const QDate &date, const WikipediaRequestContext &context, int daysBack,
                        const QVector<news_item> &fallbackItems);
    void fetchArticleContent(const QString &title, const WikipediaRequestContext &context);
};

#endif // WIKIPEDIA_HOME_CLIENT_H
