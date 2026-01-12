#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <iostream>
#include "wikipedia_client/wikipedia_client.h"
#include "components/searchbar/SearchBarModel.h"


int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("wikipedia_qt", "Main");

    Wikipedia::WikipediaClient client;
    QObject::connect(&client, &Wikipedia::WikipediaClient::searchCompleted,
                     [](const QVector<Wikipedia::SearchResult> &results) {
                         for (const auto &result : results) {
                             std::cout << "Title:" << result.title.toStdString() << "Snippet:" << result.snippet.toStdString() << "ID:" << result.pageid << std::endl;
                         }
                     });

    client.search("C++ programming language");

    return app.exec();
}
