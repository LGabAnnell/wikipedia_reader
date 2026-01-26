#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include <QLoggingCategory>
#include "GlobalState.h"
#include "HistoryState.h"
#include "NavigationState.h"
#include "SvgImageProvider.h"
#include "HeaderModel.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    #ifdef DEBUG
    QLoggingCategory::defaultCategory()->setEnabled(QtDebugMsg, true);
    #endif // DEBUG


    // Create and register HistoryState singleton
    QPointer<HistoryState> historyState = new HistoryState(&app);
    qmlRegisterSingletonInstance("wikipedia_qt", 1, 0, "HistoryState", historyState.get());

    // Create and register NavigationState singleton
    QPointer<NavigationState> navigationState = new NavigationState(&app);
    qmlRegisterSingletonInstance("wikipedia_qt", 1, 0, "NavigationState", navigationState.get());

    // Create and register GlobalState singleton
    QPointer<GlobalState> globalState = new GlobalState(&app, historyState);
    qmlRegisterSingletonInstance("wikipedia_qt", 1, 0, "GlobalState", globalState.get());

    HeaderModel headerModel;
    engine.addImageProvider("svg", new SvgImageProvider(&headerModel));


    // Load the QML application
    engine.loadFromModule("wikipedia_qt", "Main");

    qDebug() << engine.importPathList();

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}

