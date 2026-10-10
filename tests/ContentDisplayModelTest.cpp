#include "ContentDisplayModel.h"
#include "ArticleState.h"
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QTest>

class ContentDisplayModelTest : public QObject {
    Q_OBJECT

  private:
    QTemporaryDir m_dataDirectory;
    const QString m_html = "<p>Introduction</p><h2><a name=\"First\">First heading</a></h2>"
                           "<p>Some text</p><h2><a name=\"Second\">Second heading</a></h2>";

    QVariantList sections() const {
        section first{};
        first.anchor = "First";
        first.index = 1;
        section second{};
        second.anchor = "Second";
        second.index = 2;
        return {QVariant::fromValue(first), QVariant::fromValue(second)};
    }

  private slots:
    void initTestCase() {
        QVERIFY(m_dataDirectory.isValid());
        qputenv("XDG_DATA_HOME", m_dataDirectory.path().toUtf8());
        QCoreApplication::setApplicationName("ContentDisplayModelTest");
    }

    void sectionStateBelongsToEachView() {
        ContentDisplayModel first;
        ContentDisplayModel second;
        QCOMPARE(first.currentSectionIndex(), -1);
        QSignalSpy spy(&first, &ContentDisplayModel::currentSectionIndexChanged);
        first.setCurrentSectionIndex(1);
        first.setCurrentSectionIndex(1);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(second.currentSectionIndex(), -1);
        second.setCurrentSectionIndex(2);
        first.resetSectionTracking();
        QCOMPARE(first.currentSectionIndex(), -1);
        QCOMPARE(second.currentSectionIndex(), 2);
    }

    void sectionChangesResetHighlightAndPositions() {
        ContentDisplayModel model;
        model.updateSectionPositions(m_html, sections());
        const int firstPosition = model.findSectionPosition(m_html, "First");
        const int secondPosition = model.findSectionPosition(m_html, "Second");
        QVERIFY(firstPosition > 0);
        QVERIFY(secondPosition > firstPosition);
        QCOMPARE(model.findSectionAtPosition(0), -1);
        QCOMPARE(model.findSectionAtPosition(firstPosition), 1);
        QCOMPARE(model.findSectionAtPosition(secondPosition), 2);
        model.setCurrentSectionIndex(2);
        model.updateSectionPositions(m_html, {});
        QCOMPARE(model.currentSectionIndex(), -1);
        QCOMPARE(model.findSectionAtPosition(secondPosition), -1);
        model.updateSectionPositions(m_html, sections());
        model.setCurrentSectionIndex(1);
        model.updateSectionPositions({}, sections());
        QCOMPARE(model.currentSectionIndex(), -1);
        QCOMPARE(model.findSectionAtPosition(secondPosition), -1);
    }

    void articleChangesResetViewState() {
        HistoryState history;
        SettingsState settings;
        ArticleState article(history, settings);
        ContentDisplayModel model;
        model.updateSectionPositions(m_html, sections());
        model.setCurrentSectionIndex(2);
        model.performSearch("text", "text and text");
        QCOMPARE(model.totalResults(), 2);
        article.setCurrentPageFromData("Another article", "<p>Different content</p>", "");
        QCOMPARE(model.currentSectionIndex(), -1);
        QCOMPARE(model.findSectionAtPosition(10000), -1);
        QCOMPARE(model.totalResults(), 0);
        QCOMPARE(model.currentResultIndex(), 0);
    }

    void textSearchNavigationStillWraps() {
        ContentDisplayModel model;
        const auto results = model.performSearch("text", "Text and TEXT");
        QCOMPARE(results.size(), 2);
        QCOMPARE(results[0].start, 0);
        QCOMPARE(results[1].start, 9);
        QSignalSpy spy(&model, &ContentDisplayModel::navigateToResult);
        model.navigateToPreviousResult();
        QCOMPARE(model.currentResultIndex(), 2);
        QCOMPARE(spy.count(), 1);
        model.navigateToNextResult();
        QCOMPARE(model.currentResultIndex(), 1);
        model.performSearch({}, "Text and TEXT");
        QCOMPARE(model.totalResults(), 0);
    }
};

QTEST_MAIN(ContentDisplayModelTest)
#include "ContentDisplayModelTest.moc"
