#include "support/FakeNetworkAccessManager.h"
#include "wikipedia_featured_client.h"
#include "wikipedia_home_client.h"
#include "wikipedia_network_access_manager.h"
#include "wikipedia_page_client.h"
#include "wikipedia_search_client.h"
#include <QSignalSpy>
#include <QtTest/QTest>

class WikipediaRequestIsolationTest : public QObject {
    Q_OBJECT
  private:
    NetworkFixtureController m_network;

  private slots:
    void initTestCase() {
        connect(&m_network, &NetworkFixtureController::unexpectedRequest, this,
                [](const QString &request) { qFatal("%s", qPrintable(request)); });
        WikipediaNetwork::installNetworkAccessManagerFactory(&m_network);
    }

    void cleanupTestCase() { WikipediaNetwork::installNetworkAccessManagerFactory(nullptr); }
    void init() { m_network.clearRequests(); }
    void cleanup() { QCOMPARE(m_network.pendingReplyCount(), 0); }

    void searchIgnoresReplyFromPreviousLanguage() {
        WikipediaSearchClient client;
        const QUrl en("https://en.wikipedia.org/w/api.php?action=query&format=json&list=search&srsearch=old&srlimit=10");
        const QUrl fr("https://fr.wikipedia.org/w/api.php?action=query&format=json&list=search&srsearch=new&srlimit=10");
        m_network.addFixture("GET", en, R"({"query":{"search":[{"title":"Old","pageid":1}]}})");
        m_network.addFixture("GET", fr, R"({"query":{"search":[{"title":"French","pageid":2}]}})");
        QSignalSpy results(&client, &WikipediaSearchClient::searchCompleted);
        QSignalSpy errors(&client, &WikipediaSearchClient::errorOccurred);
        m_network.deferNextReply(true);
        client.search("old");
        client.setLanguage("fr");
        client.search("new");
        QTRY_COMPARE(results.count(), 1);
        m_network.completeReply(en);
        QCoreApplication::processEvents();
        QCOMPARE(results.count(), 1);
        QCOMPARE(results.first().first().value<QVector<search_result>>().first().title, QString("French"));
        QCOMPARE(errors.count(), 0);
    }

    void obsoleteSearchFailureDoesNotEmitError() {
        WikipediaSearchClient client;
        const QUrl en("https://en.wikipedia.org/w/api.php?action=query&format=json&list=search&srsearch=old&srlimit=10");
        const QUrl fr("https://fr.wikipedia.org/w/api.php?action=query&format=json&list=search&srsearch=new&srlimit=10");
        m_network.addFixture("GET", en, "old failure", "text/plain", 500);
        m_network.addFixture("GET", fr, R"({"query":{"search":[]}})");
        QSignalSpy results(&client, &WikipediaSearchClient::searchCompleted);
        QSignalSpy errors(&client, &WikipediaSearchClient::errorOccurred);
        m_network.deferNextReply(true);
        client.search("old");
        client.setLanguage("fr");
        client.search("new");
        QTRY_COMPARE(results.count(), 1);
        m_network.completeReply(en);
        QCoreApplication::processEvents();
        QCOMPARE(errors.count(), 0);
        QCOMPARE(results.count(), 1);
    }

    void pageReplyCannotStartOldEditionHtmlRequest() {
        WikipediaPageClient client;
        const QUrl en("https://en.wikipedia.org/w/api.php?action=query&format=json&prop=extracts%7Cimages&pageids=41&explaintext=1&imlimit=50");
        m_network.addFixture("GET", en,
            R"({"query":{"pages":{"41":{"pageid":41,"title":"Old","extract":"","images":[]}}}})");
        QSignalSpy pages(&client, &WikipediaPageClient::pageReceived);
        QSignalSpy errors(&client, &WikipediaPageClient::errorOccurred);
        m_network.deferNextReply(true);
        client.getPageById(41);
        client.setLanguage("fr");
        m_network.completeReply(en);
        QCoreApplication::processEvents();
        QCOMPARE(m_network.requestCount(), 1);
        QCOMPARE(pages.count(), 0);
        QCOMPARE(errors.count(), 0);
    }

    void featuredReplyFromOldEditionIsIgnored() {
        WikipediaFeaturedClient client;
        const QString date = QDate::currentDate().toString("yyyy/MM/dd");
        const QUrl en(QString("https://api.wikimedia.org/feed/v1/wikipedia/en/featured/%1").arg(date));
        const QUrl fr(QString("https://api.wikimedia.org/feed/v1/wikipedia/fr/featured/%1").arg(date));
        m_network.addFixture("GET", en, R"({"tfa":{"title":"Old","extract":"","pageid":1}})");
        m_network.addFixture("GET", fr, R"({"tfa":{"title":"French","extract":"","pageid":2}})");
        QSignalSpy received(&client, &WikipediaFeaturedClient::featuredArticleReceived);
        QSignalSpy errors(&client, &WikipediaFeaturedClient::errorOccurred);
        m_network.deferNextReply(true);
        client.getFeaturedArticleOfTheDay();
        client.setLanguage("fr");
        client.getFeaturedArticleOfTheDay();
        QTRY_COMPARE(received.count(), 1);
        m_network.completeReply(en);
        QCoreApplication::processEvents();
        QCOMPARE(received.count(), 1);
        QCOMPARE(received.first().at(0).toString(), QString("French"));
        QCOMPARE(errors.count(), 0);
    }

    void randomTitleReplyCannotStartSummaryInNewLanguage() {
        WikipediaHomeClient client;
        const QUrl en("https://en.wikipedia.org/api/rest_v1/page/random/title");
        m_network.addFixture("GET", en, R"({"items":[{"title":"Old title"}]})");
        QSignalSpy items(&client, &WikipediaHomeClient::didYouKnowItemsReceived);
        QSignalSpy errors(&client, &WikipediaHomeClient::errorOccurred);
        m_network.deferNextReply(true);
        client.getDidYouKnowItems();
        client.setLanguage("fr");
        m_network.completeReply(en);
        QCoreApplication::processEvents();
        QCOMPARE(m_network.requestCount(), 1);
        QCOMPARE(items.count(), 0);
        QCOMPARE(errors.count(), 0);
    }
};

QTEST_GUILESS_MAIN(WikipediaRequestIsolationTest)
#include "WikipediaRequestIsolationTest.moc"
