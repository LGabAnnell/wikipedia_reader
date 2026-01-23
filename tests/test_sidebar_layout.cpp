// test_sidebar_layout.cpp
#include <QtTest/QtTest>
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

    void testSidebarWidthConstraints()
    {
        // Test that Sidebar respects width constraints
        QQmlEngine engine;
        QQmlComponent component(&engine, QUrl::fromLocalFile("SidebarModule/Sidebar.qml"));
        
        if (component.isError()) {
            QFAIL("Failed to load Sidebar.qml");
        }
        
        QObject *sidebarObj = component.create();
        QVERIFY(sidebarObj != nullptr);
        
        // Check if the sidebar has the expected properties
        QQuickItem *sidebar = qobject_cast<QQuickItem*>(sidebarObj);
        QVERIFY(sidebar != nullptr);
        
        // The sidebar should have layout properties set
        QCOMPARE(sidebar->property("Layout.preferredWidth").toInt(), 200);
        QCOMPARE(sidebar->property("Layout.minimumWidth").toInt(), 40);
        QCOMPARE(sidebar->property("Layout.maximumWidth").toInt(), 200);
        
        delete sidebarObj;
    }

    void testSidebarButtons()
    {
        // Test that sidebar buttons are properly laid out
        QQmlEngine engine;
        QQmlComponent component(&engine, QUrl::fromLocalFile("SidebarModule/Sidebar.qml"));
        
        if (component.isError()) {
            QFAIL("Failed to load Sidebar.qml");
        }
        
        QObject *sidebarObj = component.create();
        QVERIFY(sidebarObj != nullptr);
        
        QQuickItem *sidebar = qobject_cast<QQuickItem*>(sidebarObj);
        QVERIFY(sidebar != nullptr);
        
        // Find the buttons
        QList<QQuickItem*> buttons = sidebar->findChildren<QQuickItem*>(QString(), Qt::FindChildrenRecursively);
        int buttonCount = 0;
        
        for (QQuickItem *item : buttons) {
            if (item->metaObject()->className() == QString("QQuickButton")) {
                buttonCount++;
                // Check that buttons have Layout.fillWidth: true
                QVERIFY(item->property("Layout.fillWidth").toBool());
            }
        }
        
        QCOMPARE(buttonCount, 3); // Should have Home, History, Bookmarks buttons
        
        delete sidebarObj;
    }

    void cleanupTestCase()
    {
        // Clean up
    }
};

QTEST_MAIN(TestSidebarLayout)

#include "test_sidebar_layout.moc"