#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <iostream>
#include "wikipedia_client/wikipedia_client.h"
#include "components/searchbar/SearchBarModel.h"
#include "GlobalState.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    // Create and register GlobalState singleton
    GlobalState *globalState = new GlobalState(&app);
    qmlRegisterSingletonInstance("wikipedia_qt", 1, 0, "GlobalState", globalState);

    // Create and register SearchBarModel singleton
    SearchBarModel *searchBarModel = new SearchBarModel(&app);
    searchBarModel->setGlobalState(globalState); // Connect SearchBarModel to GlobalState
    qmlRegisterSingletonInstance("wikipedia_qt", 1, 0, "SearchBarModel", searchBarModel);

    // Load the QML application
    engine.loadFromModule("wikipedia_qt", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
