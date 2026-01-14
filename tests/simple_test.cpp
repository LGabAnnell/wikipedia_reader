#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include "../src/wikipedia_client/wikipedia_client.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // Register custom types
    qRegisterMetaType<Wikipedia::SearchResult>();
    qRegisterMetaType<Wikipedia::Page>();
    qRegisterMetaType<Wikipedia::FeaturedArticle>();
    qRegisterMetaType<QVector<Wikipedia::SearchResult>>();

    Wikipedia::WikipediaClient client;

    // Test search
    QObject::connect(&client, &Wikipedia::WikipediaClient::searchCompleted,
        [](const QVector<Wikipedia::SearchResult> &results) {
            qDebug() << "Search completed with" << results.size() << "results:";
            for (const auto &result : results) {
                qDebug() << "  - " << result.title << "(ID:" << result.pageid << ")";
            }
            qDebug() << "Search test completed successfully!";
        });

    QObject::connect(&client, &Wikipedia::WikipediaClient::errorOccurred,
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
