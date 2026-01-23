// tests/GlobalStateTest.cpp
#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QVector>
#include "GlobalState.h"

class GlobalStateTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Initialize any resources needed for all tests
    }

    void cleanupTestCase() {
        // Clean up resources after all tests
    }

    void testInitialState() {
        GlobalState globalState;

        // Test initial state
        QVERIFY(globalState.searchResults().isEmpty());
        QVERIFY(globalState.currentPageTitle().isEmpty());
        QVERIFY(!globalState.isLoading());
    }

    void testSetSearchResults() {
        GlobalState globalState;
        QVector<search_result> results;

        // Add a test search result
        search_result result;
        result.title = "Test Title";
        result.snippet = "Test Snippet";
        result.pageid = 123;
        results.append(result);

        // Set and verify
        globalState.setSearchResults(results);
        QCOMPARE(globalState.searchResults().size(), 1);
        QCOMPARE(globalState.searchResults()[0].title, QString("Test Title"));
    }

    void testSetCurrentPage() {
        GlobalState globalState;
        page page;

        // Set page details
        page.title = "Test Page";
        page.extract = "Test Extract";
        page.pageid = 456;

        // Set and verify
        globalState.setCurrentPage(page);
        QCOMPARE(globalState.currentPageTitle(), QString("Test Page"));
        QCOMPARE(globalState.currentPageExtract(), QString("Test Extract"));
        QCOMPARE(globalState.currentPageId(), 456);
    }

    void testSetIsLoading() {
        GlobalState globalState;

        // Test loading state changes
        globalState.setIsLoading(true);
        QVERIFY(globalState.isLoading());

        globalState.setIsLoading(false);
        QVERIFY(!globalState.isLoading());
    }

    void testSignalsEmitted() {
        GlobalState globalState;

        // Use QSignalSpy to verify signals are emitted
        QSignalSpy searchResultsSpy(&globalState, &GlobalState::searchResultsChanged);
        QSignalSpy currentPageSpy(&globalState, &GlobalState::currentPageChanged);
        QSignalSpy isLoadingSpy(&globalState, &GlobalState::isLoadingChanged);

        // Trigger changes
        QVector<search_result> results;
        globalState.setSearchResults(results);
        QCOMPARE(searchResultsSpy.count(), 1);

        page page;
        globalState.setCurrentPage(page);
        QCOMPARE(currentPageSpy.count(), 1);

        globalState.setIsLoading(true);
        QCOMPARE(isLoadingSpy.count(), 1);
    }

    void testLoadArticleByPageId() {
        GlobalState globalState;

        // Test that the method exists and can be called
        QVERIFY(QMetaObject::invokeMethod(&globalState, "loadArticleByPageId", 
                                        Q_ARG(int, 736))); // Einstein's page ID
        
        // Verify loading state is set
        QVERIFY(globalState.isLoading());
        
        // Verify error message is cleared
        QVERIFY(globalState.errorMessage().isEmpty());
    }
};

QTEST_MAIN(GlobalStateTest)
#include "GlobalStateTest.moc"
