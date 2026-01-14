#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "=== Basic Network Test ===";
    
    // Test a simple network request
    QNetworkAccessManager manager;
    QUrl url("https://en.wikipedia.org/w/api.php");
    QUrlQuery query;
    query.addQueryItem("action", "query");
    query.addQueryItem("format", "json");
    query.addQueryItem("list", "search");
    query.addQueryItem("srsearch", "test");
    query.addQueryItem("srlimit", "3");
    url.setQuery(query);
    
    qDebug() << "Making request to:" << url.toString();
    
    QNetworkReply *reply = manager.get(QNetworkRequest(url));
    QObject::connect(reply, &QNetworkReply::finished, [reply, &app]() {
        qDebug() << "Request finished!";
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "Success! Response size:" << reply->bytesAvailable();
            QByteArray data = reply->readAll();
            qDebug() << "First 200 chars:" << data.left(200);
        } else {
            qDebug() << "Error:" << reply->errorString();
        }
        reply->deleteLater();
        app.quit();
    });

    return app.exec();
}
