#include "ImageSelectionState.h"
#include <QSignalSpy>
#include <QtTest/QTest>

class ImageSelectionStateTest : public QObject {
    Q_OBJECT

  private slots:
    void atomicSelection() {
        SettingsState settings;
        ImageSelectionState selection(settings);
        QVERIFY(selection.currentImageUrl().isEmpty());
        QVERIFY(selection.currentImageDescription().isEmpty());
        QSignalSpy urlSpy(&selection, &ImageSelectionState::currentImageUrlChanged);
        QSignalSpy captionSpy(&selection, &ImageSelectionState::currentImageDescriptionChanged);
        connect(&selection, &ImageSelectionState::currentImageUrlChanged, this, [&]() {
            QCOMPARE(selection.currentImageDescription(), QString("caption"));
        });
        connect(&selection, &ImageSelectionState::currentImageDescriptionChanged, this, [&]() {
            QCOMPARE(selection.currentImageUrl(), QString("image.png"));
        });
        selection.selectImage("image.png", "caption");
        QCOMPARE(urlSpy.count(), 1);
        QCOMPARE(captionSpy.count(), 1);
        selection.selectImage("image.png", "caption");
        QCOMPARE(urlSpy.count(), 1);
        QCOMPARE(captionSpy.count(), 1);
    }

    void languageTransitionClearsSelection() {
        SettingsState settings;
        ImageSelectionState selection(settings);
        selection.selectImage("old.png", "Old edition caption");
        settings.setLanguage("fr");
        QVERIFY(selection.currentImageUrl().isEmpty());
        QVERIFY(selection.currentImageDescription().isEmpty());
    }

    void teardownAndRecreation() {
        QVERIFY(ImageSelectionState::instance().isNull());
        {
            SettingsState settings;
            ImageSelectionState selection(settings);
            QCOMPARE(ImageSelectionState::instance().data(), &selection);
            selection.selectImage("image.png", "caption");
        }
        QVERIFY(ImageSelectionState::instance().isNull());
        SettingsState settings;
        ImageSelectionState selection(settings);
        QVERIFY(selection.currentImageUrl().isEmpty());
        QVERIFY(selection.currentImageDescription().isEmpty());
    }
};

QTEST_GUILESS_MAIN(ImageSelectionStateTest)
#include "ImageSelectionStateTest.moc"
