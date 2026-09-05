// tests/GlobalStateTest.cpp
#include <QCoreApplication>
#include <QSignalSpy>
#include <QtTest/QTest>
#include "GlobalState.h"
#include "HistoryState.h"

class GlobalStateTest : public QObject {
    Q_OBJECT

private:
    HistoryState *m_historyState = nullptr;

private slots:
    void initTestCase() {
        QCoreApplication::setApplicationName("GlobalStateTest");
        m_historyState = new HistoryState();
    }

    void cleanupTestCase() {
        delete m_historyState;
        m_historyState = nullptr;
    }

    void init() {
        // Clear history before each test so state doesn't leak between tests
        m_historyState->clearHistory();
    }

    // --- Original tests (fixed to pass HistoryState) ---

    void testInitialState() {
        GlobalState globalState(nullptr, m_historyState);

        QVERIFY(globalState.searchResults().isEmpty());
        QVERIFY(globalState.currentPageTitle().isEmpty());
        QVERIFY(!globalState.isLoading());
    }

    void testSetSearchResults() {
        GlobalState globalState(nullptr, m_historyState);
        QVector<search_result> results;

        search_result result;
        result.title = "Test Title";
        result.snippet = "Test Snippet";
        result.pageid = 123;
        results.append(result);

        globalState.setSearchResults(results);
        QCOMPARE(globalState.searchResults().size(), 1);
        QCOMPARE(globalState.searchResults()[0].title, QString("Test Title"));
    }

    void testSetCurrentPage() {
        GlobalState globalState(nullptr, m_historyState);
        page p;

        p.title = "Test Page";
        p.extract = "Test Extract";
        p.pageid = 456;
        p.imageUrls = QStringList();

        globalState.setCurrentPage(p);
        QCOMPARE(globalState.currentPageTitle(), QString("Test Page"));
        QCOMPARE(globalState.currentPageExtract(), QString("Test Extract"));
        QCOMPARE(globalState.currentPageId(), 456);
    }

    void testSetIsLoading() {
        GlobalState globalState(nullptr, m_historyState);

        globalState.setIsLoading(true);
        QVERIFY(globalState.isLoading());

        globalState.setIsLoading(false);
        QVERIFY(!globalState.isLoading());
    }

    void testSignalsEmitted() {
        GlobalState globalState(nullptr, m_historyState);

        QSignalSpy searchResultsSpy(&globalState, &GlobalState::searchResultsChanged);
        QSignalSpy currentPageSpy(&globalState, &GlobalState::currentPageChanged);
        QSignalSpy isLoadingSpy(&globalState, &GlobalState::isLoadingChanged);

        QVector<search_result> results;
        globalState.setSearchResults(results);
        QCOMPARE(searchResultsSpy.count(), 1);

        page p;
        p.imageUrls = QStringList();
        globalState.setCurrentPage(p);
        QCOMPARE(currentPageSpy.count(), 1);

        globalState.setIsLoading(true);
        QCOMPARE(isLoadingSpy.count(), 1);
    }

    void testLoadArticleByPageId() {
        GlobalState globalState(nullptr, m_historyState);

        QVERIFY(QMetaObject::invokeMethod(&globalState, "loadArticleByPageId",
                                          Q_ARG(int, 736)));

        QVERIFY(globalState.isLoading());
        QVERIFY(globalState.errorMessage().isEmpty());
    }

    // --- Phase 3: Expanded tests ---

    void testSetSections() {
        GlobalState globalState(nullptr, m_historyState);

        QSignalSpy spy(&globalState, &GlobalState::sectionsChanged);

        QVector<section> sections;
        section s;
        s.title = "History";
        s.level = 1;
        s.anchor = "History";
        s.index = 1;
        sections.append(s);

        globalState.setSections(sections);
        QCOMPARE(globalState.currentPageSections().size(), 1);
        QCOMPARE(globalState.currentPageSections()[0].title, QString("History"));
        QCOMPARE(spy.count(), 1);
    }

    void testSetLoadingSections() {
        GlobalState globalState(nullptr, m_historyState);

        QSignalSpy spy(&globalState, &GlobalState::loadingSectionsChanged);

        globalState.setLoadingSections(true);
        QVERIFY(globalState.isLoadingSections());
        QCOMPARE(spy.count(), 1);

        globalState.setLoadingSections(false);
        QVERIFY(!globalState.isLoadingSections());
        QCOMPARE(spy.count(), 2);
    }

    void testSetErrorMessage() {
        GlobalState globalState(nullptr, m_historyState);

        QSignalSpy spy(&globalState, &GlobalState::errorMessageChanged);

        globalState.setErrorMessage("Something went wrong");
        QCOMPARE(globalState.errorMessage(), QString("Something went wrong"));
        QCOMPARE(spy.count(), 1);
    }

    void testClearErrorMessage() {
        GlobalState globalState(nullptr, m_historyState);

        QSignalSpy spy(&globalState, &GlobalState::errorMessageChanged);

        globalState.setErrorMessage("Error");
        QCOMPARE(spy.count(), 1);

        globalState.clearErrorMessage();
        QVERIFY(globalState.errorMessage().isEmpty());
        QCOMPARE(spy.count(), 2);
    }

    void testSetCurrentImageUrl() {
        GlobalState globalState(nullptr, m_historyState);

        QSignalSpy spy(&globalState, &GlobalState::currentImageUrlChanged);

        globalState.setCurrentImageUrl("https://example.com/image.jpg");
        QCOMPARE(globalState.currentImageUrl(), QString("https://example.com/image.jpg"));
        QCOMPARE(spy.count(), 1);
    }

    void testSetCurrentImageDescription() {
        GlobalState globalState(nullptr, m_historyState);

        QSignalSpy spy(&globalState, &GlobalState::currentImageDescriptionChanged);

        globalState.setCurrentImageDescription("A test image");
        QCOMPARE(globalState.currentImageDescription(), QString("A test image"));
        QCOMPARE(spy.count(), 1);
    }

    void testCurrentImageUrlNoDuplicateSignal() {
        GlobalState globalState(nullptr, m_historyState);

        QSignalSpy spy(&globalState, &GlobalState::currentImageUrlChanged);

        globalState.setCurrentImageUrl("https://example.com/image.jpg");
        QCOMPARE(spy.count(), 1);

        // Setting the same value should not emit
        globalState.setCurrentImageUrl("https://example.com/image.jpg");
        QCOMPARE(spy.count(), 1);
    }

    void testCurrentImageDescriptionNoDuplicateSignal() {
        GlobalState globalState(nullptr, m_historyState);

        QSignalSpy spy(&globalState, &GlobalState::currentImageDescriptionChanged);

        globalState.setCurrentImageDescription("Same description");
        QCOMPARE(spy.count(), 1);

        globalState.setCurrentImageDescription("Same description");
        QCOMPARE(spy.count(), 1);
    }

    void testSetCurrentPageFromData() {
        GlobalState globalState(nullptr, m_historyState);

        QSignalSpy spy(&globalState, &GlobalState::currentPageChanged);

        globalState.setCurrentPageFromData("Test Title", "Test Extract", "https://example.com");
        QCOMPARE(globalState.currentPageTitle(), QString("Test Title"));
        QCOMPARE(globalState.currentPageExtract(), QString("Test Extract"));
        QCOMPARE(globalState.currentPageId(), 0);
        QCOMPARE(spy.count(), 1);
    }

    void testSetCurrentPageCaching() {
        GlobalState globalState(nullptr, m_historyState);

        // Populate the cache by setting a page with pageid > 0
        page p;
        p.title = "Cached Article";
        p.extract = "Cached extract";
        p.pageid = 999;
        p.imageUrls = QStringList();
        globalState.setCurrentPage(p);

        // Now loadArticleByPageId with the same id should be a cache hit
        // (loading stays false, no network request)
        globalState.setIsLoading(true); // Set loading true to verify it gets set false
        globalState.loadArticleByPageId(999);
        QVERIFY(!globalState.isLoading());
        QCOMPARE(globalState.currentPageTitle(), QString("Cached Article"));
    }

    void testSetCurrentPageAddsToHistory() {
        GlobalState globalState(nullptr, m_historyState);

        QSignalSpy spy(m_historyState, &HistoryState::historyChanged);

        page p;
        p.title = "History Test";
        p.extract = "Extract";
        p.pageid = 777;
        p.imageUrls = QStringList();
        globalState.setCurrentPage(p);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(m_historyState->history().size(), 1);
        QCOMPARE(m_historyState->history()[0].title, QString("History Test"));
        QCOMPARE(m_historyState->history()[0].pageId, 777);
    }

    void testHandleArticleLoadError() {
        GlobalState globalState(nullptr, m_historyState);

        globalState.setIsLoading(true);
        QVERIFY(globalState.isLoading());

        // handleArticleLoadError is a private slot — invoke via QMetaObject
        QVERIFY(QMetaObject::invokeMethod(&globalState, "handleArticleLoadError",
                                          Q_ARG(QString, "Network error")));

        QVERIFY(!globalState.isLoading());
        QCOMPARE(globalState.errorMessage(), QString("Network error"));
    }
};

QTEST_MAIN(GlobalStateTest)
#include "GlobalStateTest.moc"
