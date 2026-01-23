#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QVector>
#include "wikipedia_client.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // Register custom types
    qRegisterMetaType<search_result>();
    qRegisterMetaType<page>();
    qRegisterMetaType<featured_article>();
    qRegisterMetaType<QVector<search_result>>();

    WikipediaClient client;

    // Test search
    QObject::connect(&client, &WikipediaClient::searchCompleted,
        [](const QVector<search_result> &results) {
            qDebug() << "Search completed with" << results.size() << "results:";
            for (const auto &result : results) {
                qDebug() << "  - " << result.title << "(ID:" << result.pageid << ")";
            }
            qDebug() << "Search test completed successfully!";
        });

    QObject::connect(&client, &WikipediaClient::errorOccurred,
        [](const QString &error) {
            qDebug() << "Error occurred:" << error;
        });

    qDebug() << "Starting Wikipedia client test...";
    qDebug() << "Testing search for 'test'...";
    client.search("test");

    qDebug() << "Search request sent, waiting for response...";

    // Quit after a delay
    QTimer::singleShot(5000, &app, &QCoreApplication::quit);

    return app.exec();
}
