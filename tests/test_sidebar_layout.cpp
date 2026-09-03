// test_sidebar_layout.cpp
#include <QtTest/QtTest>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickView>
#include <QtQuickControls2/QQuickStyle>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>

#include "SidebarModel.h"

class TestSidebarLayout : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Initialize QML engine
        qmlRegisterType<SidebarModel>("Sidebar", 1, 0, "SidebarModel");
    }

    void testSidebarLoads()
    {
        // Verify that Sidebar.qml loads successfully
        QQmlEngine engine;
        QQmlComponent component(&engine, QUrl::fromLocalFile("SidebarModule/Sidebar.qml"));

        if (component.isError()) {
            for (const auto &err : component.errors()) {
                qWarning() << err.toString();
            }
            QFAIL("Failed to load Sidebar.qml");
        }

        QObject *sidebarObj = component.create();
        QVERIFY(sidebarObj != nullptr);

        QQuickItem *sidebar = qobject_cast<QQuickItem*>(sidebarObj);
        QVERIFY(sidebar != nullptr);

        delete sidebarObj;
    }

    void testSidebarHasListView()
    {
        // Verify that the sidebar contains a ListView child
        QQmlEngine engine;
        QQmlComponent component(&engine, QUrl::fromLocalFile("SidebarModule/Sidebar.qml"));

        if (component.isError()) {
            QFAIL("Failed to load Sidebar.qml");
        }

        QObject *sidebarObj = component.create();
        QVERIFY(sidebarObj != nullptr);

        QQuickItem *sidebar = qobject_cast<QQuickItem*>(sidebarObj);
        QVERIFY(sidebar != nullptr);

        // Find ListView children
        QList<QQuickItem*> children = sidebar->findChildren<QQuickItem*>(QString(), Qt::FindChildrenRecursively);
        bool foundListView = false;
        for (QQuickItem *item : children) {
            if (item->metaObject()->className() == QString("QQuickListView")) {
                foundListView = true;
                break;
            }
        }
        QVERIFY2(foundListView, "Sidebar should contain a ListView");

        delete sidebarObj;
    }

    void testSidebarHasSearchResultsProperty()
    {
        // Verify that the sidebar exposes a searchResults property
        QQmlEngine engine;
        QQmlComponent component(&engine, QUrl::fromLocalFile("SidebarModule/Sidebar.qml"));

        if (component.isError()) {
            QFAIL("Failed to load Sidebar.qml");
        }

        QObject *sidebarObj = component.create();
        QVERIFY(sidebarObj != nullptr);

        QVERIFY(sidebarObj->metaObject()->indexOfProperty("searchResults") != -1);

        delete sidebarObj;
    }

    void cleanupTestCase()
    {
        // Clean up
    }
};

QTEST_MAIN(TestSidebarLayout)

#include "test_sidebar_layout.moc"