// wikipedia_search_client.h
#ifndef WIKIPEDIA_SEARCH_CLIENT_H
#define WIKIPEDIA_SEARCH_CLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QVector>
#include "wikipedia_models.h"

class WikipediaSearchClient : public QObject
{
    Q_OBJECT
public:
    explicit WikipediaSearchClient(QObject *parent = nullptr);
    ~WikipediaSearchClient();

    void search(const QString &query, int limit = 10);

signals:
    void searchCompleted(const QVector<search_result> &results);
    void errorOccurred(const QString &error);

private slots:
    void onSearchReply(QNetworkReply *reply);

private:
    QNetworkAccessManager *networkManager;
    QString baseUrl;
};

#endif // WIKIPEDIA_SEARCH_CLIENT_H
