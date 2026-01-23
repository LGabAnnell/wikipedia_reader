#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QVector>
#include "wikipedia_client.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // Register custom types
    qRegisterMetaType<search_result>();
    qRegisterMetaType<page>();
    qRegisterMetaType<featured_article>();
    qRegisterMetaType<QVector<search_result>>();

    qDebug() << "=== Wikipedia Client Verbose Test ===";
    
    // First, let's test a direct API call to see if the Wikipedia API is working
    qDebug() << "Testing direct Wikipedia API call...";
    
    QNetworkAccessManager manager;
    QUrl url("https://en.wikipedia.org/w/api.php");
    QUrlQuery query;
    query.addQueryItem("action", "query");
    query.addQueryItem("format", "json");
    query.addQueryItem("list", "search");
    query.addQueryItem("srsearch", "test");
    query.addQueryItem("srlimit", "3");
    url.setQuery(query);
    
    QNetworkReply *reply = manager.get(QNetworkRequest(url));
    QObject::connect(reply, &QNetworkReply::finished, [reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "Direct API call successful!";
            qDebug() << "Response size:" << reply->bytesAvailable();
            qDebug() << "Response:" << reply->readAll();
        } else {
            qDebug() << "Direct API call failed:" << reply->errorString();
        }
        reply->deleteLater();
    });

    // Now test the Wikipedia client
    qDebug() << "\nTesting Wikipedia client...";
    WikipediaClient client;
    
    QObject::connect(&client, &WikipediaClient::searchCompleted, 
        [](const QVector<search_result> &results) {
            qDebug() << "Wikipedia client search completed with" << results.size() << "results:";
            for (const auto &result : results) {
                qDebug() << "  - " << result.title << "(ID:" << result.pageid << ")";
            }
        });

    QObject::connect(&client, &WikipediaClient::errorOccurred, 
        [](const QString &error) {
            qDebug() << "Wikipedia client error:" << error;
        });

    client.search("test", 3);
    
    // Quit after a delay
    QTimer::singleShot(10000, &app, &QCoreApplication::quit);
    
    return app.exec();
}
