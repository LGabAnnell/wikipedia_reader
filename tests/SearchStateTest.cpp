#include "SearchState.h"
#include "support/FakeNetworkAccessManager.h"
#include "wikipedia_network_access_manager.h"
#include <QSignalSpy>
#include <QtTest/QTest>

class SearchStateTest : public QObject {
    Q_OBJECT

  private:
    NetworkFixtureController m_network;
    QUrl searchUrl(const QString &language = "en") const {
        return QUrl("https://" + language + ".wikipedia.org/w/api.php?action=query&format=json"
                    "&list=search&srsearch=fixture&srlimit=10");
    }

  private slots:
    void initTestCase() {
        connect(&m_network, &NetworkFixtureController::unexpectedRequest, this,
                [](const QString &request) { qFatal("%s", qPrintable(request)); });
        WikipediaNetwork::installNetworkAccessManagerFactory(&m_network);
    }

    void cleanupTestCase() {
        WikipediaNetwork::installNetworkAccessManagerFactory(nullptr);
    }

    void init() {
        m_network.clearRequests();
        QVERIFY(SearchState::instance().isNull());
    }

    void cleanup() {
        QVERIFY(SearchState::instance().isNull());
        QCOMPARE(m_network.pendingReplyCount(), 0);
    }

    void initialStateAndBlankQueries() {
        SettingsState settings;
        SearchState search(settings);
        QCOMPARE(SearchState::instance().data(), &search);
        QVERIFY(search.searchResults().isEmpty());
        QVERIFY(search.errorMessage().isEmpty());
        QVERIFY(!search.isSearching());
        QVERIFY(!search.hasCompletedSearch());
        search.search("");
        search.search("  \t\n");
        QCOMPARE(m_network.requestCount(), 0);
    }

    void pendingCompletionAndReset() {
        SettingsState settings;
        SearchState search(settings);
        QSignalSpy searchingSpy(&search, &SearchState::isSearchingChanged);
        QSignalSpy completionSpy(&search, &SearchState::hasCompletedSearchChanged);
        QSignalSpy resultsSpy(&search, &SearchState::searchResultsChanged);
        m_network.deferNextReply();
        search.search("  fixture  ");
        QVERIFY(search.isSearching());
        QVERIFY(!search.hasCompletedSearch());
        QCOMPARE(searchingSpy.count(), 1);
        QCOMPARE(m_network.requestCount(), 1);
        search.search("fixture");
        search.reset();
        QVERIFY(search.isSearching());
        QCOMPARE(m_network.requestCount(), 1);
        m_network.completeNextReply();
        QTRY_VERIFY(!search.isSearching());
        QVERIFY(search.hasCompletedSearch());
        QCOMPARE(search.searchResults().size(), 1);
        QCOMPARE(search.searchResults().first().title, QString("Fixture article"));
        QCOMPARE(searchingSpy.count(), 2);
        QCOMPARE(completionSpy.count(), 1);
        QCOMPARE(resultsSpy.count(), 2);
        search.reset();
        QVERIFY(!search.hasCompletedSearch());
        QVERIFY(search.searchResults().isEmpty());
        QVERIFY(search.errorMessage().isEmpty());
        QCOMPARE(completionSpy.count(), 2);
    }

    void failureAndRetry() {
        SettingsState settings;
        SearchState search(settings);
        m_network.addFixture("GET", searchUrl(), "failure", "text/plain", 500);
        QSignalSpy errorSpy(&search, &SearchState::errorOccurred);
        search.search("fixture");
        QTRY_VERIFY(!search.isSearching());
        QVERIFY(!search.hasCompletedSearch());
        QCOMPARE(search.errorMessage(), QString("HTTP 500"));
        QCOMPARE(errorSpy.count(), 1);
        m_network.addFixture("GET", searchUrl(), R"({"query":{"search":[]}})");
        m_network.deferNextReply();
        search.search("fixture");
        QVERIFY(search.errorMessage().isEmpty());
        QVERIFY(!search.hasCompletedSearch());
        m_network.completeNextReply();
        QTRY_VERIFY(!search.isSearching());
        QVERIFY(search.hasCompletedSearch());
        QVERIFY(search.searchResults().isEmpty());
    }

    void settingsAtConstructionAndLanguageChanges() {
        SettingsState settings;
        settings.setLanguage("fr");
        SearchState search(settings);
        m_network.addFixture("GET", searchUrl("fr"), R"({"query":{"search":[]}})");
        m_network.addFixture("GET", searchUrl("de"), R"({"query":{"search":[]}})");
        search.search("fixture");
        QTRY_VERIFY(!search.isSearching());
        QCOMPARE(m_network.requests().first().toMap().value("host").toString(), QString("fr.wikipedia.org"));
        settings.setLanguage("de");
        QCOMPARE(m_network.requestCount(), 1);
        search.search("fixture");
        QTRY_VERIFY(!search.isSearching());
        QCOMPARE(m_network.requests().last().toMap().value("host").toString(), QString("de.wikipedia.org"));
    }

    void teardownAndRecreation() {
        SettingsState settings;
        {
            SearchState search(settings);
            search.setSearchResults({search_result{}});
        }
        QVERIFY(SearchState::instance().isNull());
        SearchState search(settings);
        QCOMPARE(SearchState::instance().data(), &search);
        QVERIFY(search.searchResults().isEmpty());
        QVERIFY(!search.hasCompletedSearch());
    }
};

QTEST_GUILESS_MAIN(SearchStateTest)
#include "SearchStateTest.moc"
