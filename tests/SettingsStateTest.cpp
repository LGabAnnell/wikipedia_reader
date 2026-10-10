#include "SettingsState.h"
#include <QSignalSpy>
#include <QtTest/QTest>

class SettingsStateTest : public QObject {
    Q_OBJECT

  private slots:
    void languageNotifications() {
        SettingsState settings;
        QCOMPARE(SettingsState::instance().data(), &settings);
        QCOMPARE(settings.language(), QString("en"));
        QSignalSpy spy(&settings, &SettingsState::languageChanged);
        settings.setLanguage("fr");
        QCOMPARE(settings.language(), QString("fr"));
        QCOMPARE(spy.count(), 1);
        settings.setLanguage("fr");
        QCOMPARE(spy.count(), 1);
    }

    void teardownAndRecreation() {
        QVERIFY(SettingsState::instance().isNull());
        {
            SettingsState settings;
            settings.setLanguage("de");
        }
        QVERIFY(SettingsState::instance().isNull());
        SettingsState settings;
        QCOMPARE(SettingsState::instance().data(), &settings);
        QCOMPARE(settings.language(), QString("en"));
    }
};

QTEST_GUILESS_MAIN(SettingsStateTest)
#include "SettingsStateTest.moc"
