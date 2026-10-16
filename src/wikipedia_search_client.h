// wikipedia_search_client.h
#ifndef WIKIPEDIA_SEARCH_CLIENT_H
#define WIKIPEDIA_SEARCH_CLIENT_H

#include "wikipedia_models.h"
#include "wikipedia_request_context.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QVector>

class WikipediaSearchClient : public QObject {
    Q_OBJECT
  public:
    explicit WikipediaSearchClient(QObject *parent = nullptr);
    ~WikipediaSearchClient();

    void search(const QString &query, int limit = 10);
    void setLanguage(const QString &langCode);
    void cancelPendingRequests();

    static QVector<search_result> parseSearchResults(const QByteArray &responseData);

  signals:
    void searchCompleted(const QVector<search_result> &results);
    void errorOccurred(const QString &error);

  private slots:
    void onSearchReply(QNetworkReply *reply, const WikipediaRequestContext &context);

  private:
    QNetworkAccessManager *networkManager;
    QString baseUrl;
    WikipediaRequestTracker m_requests;
};

#endif // WIKIPEDIA_SEARCH_CLIENT_H
