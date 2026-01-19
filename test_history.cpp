#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include "src/GlobalState.h"

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

    // Test history functionality
    globalState->addToHistory("Test Article", 12345);
    globalState->addToHistory("Another Article", 67890);
    
    qDebug() << "History count:" << globalState->history().size();
    for (const auto &item : globalState->history()) {
        qDebug() << "Title:" << item.title << "PageId:" << item.pageId;
    }

    // Load the QML application
    engine.loadFromModule("wikipedia_qt", "Main");

    if (engine.rootObjects().isEmpty()) {
        qDebug() << "Failed to load QML application";
        return -1;
    }

    return app.exec();
}