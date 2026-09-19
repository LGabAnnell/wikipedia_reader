#include "support/FakeNetworkAccessManager.h"
#include "support/QmlTestSupport.h"

#include "GlobalState.h"
#include "HistoryState.h"
#include "NavigationState.h"
#include "SvgImageProvider.h"
#include "HeaderModel.h"
#include "wikipedia_network_access_manager.h"

#include <QCoreApplication>
#include <QDir>
#include <QPointer>
#include <QQmlEngine>
#include <QQmlContext>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QtQuickTest/quicktest.h>

class QmlTestSetup final : public QObject {
    Q_OBJECT

  public:
    QmlTestSetup() : m_testSupport(&m_networkController) {
        if (!m_dataDirectory.isValid()) {
            qFatal("Could not create temporary QML test data directory");
        }
        qputenv("XDG_DATA_HOME", m_dataDirectory.path().toUtf8());
        connect(&m_networkController, &NetworkFixtureController::unexpectedRequest, this,
                [](const QString &description) { qFatal("%s", qPrintable(description)); },
                Qt::DirectConnection);
    }

    ~QmlTestSetup() override {
        delete m_globalState.data();
        delete m_navigationState.data();
        delete m_historyState.data();
        WikipediaNetwork::installNetworkAccessManagerFactory(nullptr);
    }

  public slots:
    void applicationAvailable() {
        QCoreApplication::setOrganizationName(QStringLiteral("WikipediaReaderTests"));
        QCoreApplication::setApplicationName(QStringLiteral("QmlTests"));
        const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (!appDataPath.startsWith(m_dataDirectory.path())) {
            qFatal("QML test app data escaped its temporary directory: %s", qPrintable(appDataPath));
        }

        WikipediaNetwork::installNetworkAccessManagerFactory(&m_networkController);

        // Register the singleton instances before any QML engine imports the
        // wikipedia_qt module; importing the module first would finalize and
        // protect it, rejecting these registrations.
        auto *application = QCoreApplication::instance();
        m_historyState = new HistoryState(application);
        const int historyTypeId =
            qmlRegisterSingletonInstance("wikipedia_qt", 1, 0, "HistoryState", m_historyState.data());
        if (historyTypeId < 0) {
            qFatal("Could not register QML singleton HistoryState");
        }

        m_navigationState = new NavigationState(application);
        const int navigationTypeId =
            qmlRegisterSingletonInstance("wikipedia_qt", 1, 0, "NavigationState", m_navigationState.data());
        if (navigationTypeId < 0) {
            qFatal("Could not register QML singleton NavigationState");
        }

        m_globalState = new GlobalState(application, m_historyState);
        const int globalTypeId =
            qmlRegisterSingletonInstance("wikipedia_qt", 1, 0, "GlobalState", m_globalState.data());
        if (globalTypeId < 0) {
            qFatal("Could not register QML singleton GlobalState");
        }
    }

    void qmlEngineAvailable(QQmlEngine *engine) {
        const QString importRoot = QStringLiteral(QML_TEST_IMPORT_PATH);
        const QString moduleRoot = QDir(importRoot).filePath(QStringLiteral("wikipedia_qt"));
        engine->addImportPath(importRoot);
        engine->addPluginPath(moduleRoot);
        for (const QString &module : {QStringLiteral("SearchBar"), QStringLiteral("Sidebar"),
                                      QStringLiteral("History"), QStringLiteral("Header"),
                                      QStringLiteral("Home"), QStringLiteral("ContentDisplay"),
                                      QStringLiteral("ImageDisplay"), QStringLiteral("Section")}) {
            engine->addPluginPath(QDir(moduleRoot).filePath(module));
        }
        engine->setNetworkAccessManagerFactory(&m_networkController);
        engine->rootContext()->setContextProperty(QStringLiteral("testSupport"), &m_testSupport);
        engine->rootContext()->setContextProperty(QStringLiteral("networkFixtures"), &m_networkController);
        engine->addImageProvider(QStringLiteral("svg"), new SvgImageProvider(&m_headerModel));
        connect(engine, &QQmlEngine::warnings, &m_testSupport, &QmlTestSupport::recordQmlWarnings);
    }

  private:
    QTemporaryDir m_dataDirectory;
    NetworkFixtureController m_networkController;
    QmlTestSupport m_testSupport;
    HeaderModel m_headerModel;
    QPointer<HistoryState> m_historyState;
    QPointer<NavigationState> m_navigationState;
    QPointer<GlobalState> m_globalState;
};

QUICK_TEST_MAIN_WITH_SETUP(wikipedia_qt, QmlTestSetup)

#include "qml_test_main.moc"
