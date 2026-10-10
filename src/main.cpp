#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDebug>
#include <QIcon>
#include <QLoggingCategory>
#include "ArticleState.h"
#include "SearchState.h"
#include "SettingsState.h"
#include "ImageSelectionState.h"
#include "ClipboardHelper.h"
#include "HistoryState.h"
#include "NavigationState.h"
#include "SvgImageProvider.h"
#include "HeaderModel.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icons/app_icon.svg"));
    app.setApplicationDisplayName("Wikipedia Reader");

    // Dependencies outlive their consumers, and all states outlive the QML engine.
    HistoryState historyState;
    SettingsState settingsState;
    ImageSelectionState imageSelectionState;
    ArticleState articleState(historyState, settingsState);
    SearchState searchState(settingsState);
    ClipboardHelper clipboardHelper;
    NavigationState navigationState;
    HeaderModel headerModel;
    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

#ifdef DEBUG
    QLoggingCategory::defaultCategory()->setEnabled(QtDebugMsg, true);
#endif // DEBUG

    qmlRegisterSingletonInstance("wikipedia_qt", 1, 0, "HistoryState", &historyState);
    qmlRegisterSingletonInstance("wikipedia_qt", 1, 0, "SettingsState", &settingsState);
    qmlRegisterSingletonInstance("wikipedia_qt", 1, 0, "ImageSelectionState", &imageSelectionState);
    qmlRegisterSingletonInstance("wikipedia_qt", 1, 0, "ArticleState", &articleState);
    qmlRegisterSingletonInstance("wikipedia_qt", 1, 0, "SearchState", &searchState);
    qmlRegisterSingletonInstance("wikipedia_qt", 1, 0, "ClipboardHelper", &clipboardHelper);
    qmlRegisterSingletonInstance("wikipedia_qt", 1, 0, "NavigationState", &navigationState);
    engine.addImageProvider("svg", new SvgImageProvider(&headerModel));
    engine.loadFromModule("wikipedia_qt", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;
    return app.exec();
}
