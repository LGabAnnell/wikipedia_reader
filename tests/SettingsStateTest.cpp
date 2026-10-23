#include "SettingsState.h"
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QTest>

class SettingsStateTest : public QObject {
    Q_OBJECT

    QTemporaryDir m_configDirectory;

  private slots:
    void initTestCase() {
        QVERIFY(m_configDirectory.isValid());
        qputenv("XDG_CONFIG_HOME", m_configDirectory.path().toUtf8());
    }

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

    void linkPreviewPreferencePersists() {
        QVERIFY(SettingsState::instance().isNull());
        {
            SettingsState settings;
            QVERIFY(settings.linkPreviewsEnabled());
            QSignalSpy spy(&settings, &SettingsState::linkPreviewsEnabledChanged);
            settings.setLinkPreviewsEnabled(false);
            QVERIFY(!settings.linkPreviewsEnabled());
            QCOMPARE(spy.count(), 1);
            settings.setLinkPreviewsEnabled(false);
            QCOMPARE(spy.count(), 1);
        }
        SettingsState restored;
        QVERIFY(!restored.linkPreviewsEnabled());
        restored.setLinkPreviewsEnabled(true);
    }
};

QTEST_GUILESS_MAIN(SettingsStateTest)
#include "SettingsStateTest.moc"
