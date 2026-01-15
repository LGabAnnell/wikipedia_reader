#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <iostream>
#include "wikipedia_client/wikipedia_client.h"
#include "SearchBarModel.h"
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

    std::cout << engine.importPathList().join('\n').toStdString() << std::endl;

    // Load the QML application
    engine.loadFromModule("wikipedia_qt", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
