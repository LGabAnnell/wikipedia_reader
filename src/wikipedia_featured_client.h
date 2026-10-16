// wikipedia_featured_client.h
#ifndef WIKIPEDIA_FEATURED_CLIENT_H
#define WIKIPEDIA_FEATURED_CLIENT_H

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

class WikipediaFeaturedClient : public QObject {
    Q_OBJECT
  public:
    explicit WikipediaFeaturedClient(QObject *parent = nullptr);
    ~WikipediaFeaturedClient();

    void getFeaturedArticleOfTheDay();
    void setLanguage(const QString &langCode);
    void cancelPendingRequests();

    static void parseFeaturedArticle(const QByteArray &responseData,
                                     QString &title, QString &extract, int &pageid);

  signals:
    void featuredArticleReceived(const QString &title, const QString &extract, const int &pageid);
    void errorOccurred(const QString &error);

  private slots:
    void onFeaturedArticleReply(QNetworkReply *reply, const WikipediaRequestContext &context);

  private:
    QNetworkAccessManager *networkManager;
    WikipediaRequestTracker m_requests;
};

#endif // WIKIPEDIA_FEATURED_CLIENT_H
