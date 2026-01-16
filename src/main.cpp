#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
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
    QPointer<GlobalState> globalState = new GlobalState(&app);
    qmlRegisterSingletonInstance("wikipedia_qt", 1, 0, "GlobalState", globalState.get());

    qDebug() << engine.importPathList();

    // Load the QML application
    engine.loadFromModule("wikipedia_qt", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
