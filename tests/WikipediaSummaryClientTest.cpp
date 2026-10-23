#include <QtTest/QtTest>
#include <QJsonDocument>
#include "wikipedia_summary_client.h"
#include "wikipedia_network_access_manager.h"
#include "support/FakeNetworkAccessManager.h"

class WikipediaSummaryClientTest final : public QObject {
    Q_OBJECT
  private:
    NetworkFixtureController m_network;
  private slots:
    void initTestCase() {
        connect(&m_network, &NetworkFixtureController::unexpectedRequest, this,
                [](const QString &url) { qFatal("%s", qPrintable(url)); });
        WikipediaNetwork::installNetworkAccessManagerFactory(&m_network);
    }
    void cleanupTestCase() { WikipediaNetwork::installNetworkAccessManagerFactory(nullptr); }
    void init() { m_network.clearRequests(); }
    void cleanup() { QCOMPARE(m_network.pendingReplyCount(), 0); }

    void parsesSummaryAndNamespaceShape() {
        const auto summary = WikipediaSummaryClient::parseSummary(R"JSON({
            "title":"Star Trek: Picard","pageid":123,"namespace":{"id":0,"text":""},
            "extract":"Intro","extract_html":"<p><b>Intro</b></p>",
            "thumbnail":{"source":"https:\/\/upload.wikimedia.org/thumb.jpg"}
        })JSON");
        QCOMPARE(summary.value("title").toString(), QString("Star Trek: Picard"));
        QCOMPARE(summary.value("pageId").toInt(), 123);
        QCOMPARE(summary.value("namespace").toInt(), 0);
        QCOMPARE(summary.value("extract").toString(), QString("Intro"));
        QCOMPARE(summary.value("thumbnail").toString(), QString("https://upload.wikimedia.org/thumb.jpg"));
        QVERIFY(summary.value("html").toString().contains("<b>Intro</b>"));
        QVERIFY(WikipediaSummaryClient::parseSummary("not json").isEmpty());
        QVERIFY(WikipediaSummaryClient::parseSummary("[]").isEmpty());
        QVERIFY(WikipediaSummaryClient::parseSummary(
            R"JSON({"title":"No id","extract":"missing page id"})JSON").isEmpty());
        QVERIFY(WikipediaSummaryClient::parseSummary(
            R"JSON({"title":"No text","pageid":2,"namespace":{"id":0}})JSON").isEmpty());
    }

    void encodesTitleAndEmitsSummary() {
        const QString title = QStringLiteral("Star Trek: Picard/100%");
        const QUrl url(QStringLiteral("https://en.wikipedia.org/api/rest_v1/page/summary/Star%20Trek%3A%20Picard%2F100%25"));
        m_network.addFixture("GET", url, R"JSON({"title":"Star Trek: Picard/100%","pageid":123,
            "namespace":{"id":0},"extract":"Intro"})JSON");
        WikipediaSummaryClient client;
        QSignalSpy received(&client, &WikipediaSummaryClient::summaryReceived);
        QSignalSpy errors(&client, &WikipediaSummaryClient::errorOccurred);
        client.fetchSummary(title);
        QTRY_COMPARE(received.count(), 1);
        QCOMPARE(errors.count(), 0);
        QCOMPARE(m_network.requestCount(), 1);
        QCOMPARE(m_network.requests().first().toMap().value("path").toString(),
                 QString("/api/rest_v1/page/summary/Star Trek: Picard/100%"));
        const auto summary = received.first().at(1).toMap();
        QCOMPARE(summary.value("pageId").toInt(), 123);
    }

    void rejectsFailedHttpStatus() {
        const QUrl url(QStringLiteral("https://en.wikipedia.org/api/rest_v1/page/summary/Missing"));
        m_network.addFixture("GET", url, R"JSON({"title":"Missing","pageid":1,"extract":"x"})JSON",
                             "application/json", 404);
        WikipediaSummaryClient client;
        QSignalSpy errors(&client, &WikipediaSummaryClient::errorOccurred);
        client.fetchSummary("Missing");
        QTRY_COMPARE(errors.count(), 1);
    }

    void cancellationRejectsDeferredReply() {
        const QUrl url(QStringLiteral("https://en.wikipedia.org/api/rest_v1/page/summary/Deferred"));
        m_network.addFixture("GET", url, R"JSON({"title":"Deferred","pageid":2,"extract":"x"})JSON");
        m_network.deferNextReply(true);
        WikipediaSummaryClient client;
        QSignalSpy received(&client, &WikipediaSummaryClient::summaryReceived);
        client.fetchSummary("Deferred");
        QTRY_COMPARE(m_network.pendingReplyCount(), 1);
        client.cancelPendingRequests();
        m_network.completeNextReply();
        QTest::qWait(10);
        QCOMPARE(received.count(), 0);
    }

    void timeoutEmitsUnavailableForDeferredRequest() {
        const QUrl url(QStringLiteral("https://en.wikipedia.org/api/rest_v1/page/summary/Slow"));
        m_network.addFixture("GET", url, R"JSON({"title":"Slow","pageid":3,"extract":"x"})JSON");
        m_network.deferNextReply();
        WikipediaSummaryClient client;
        QSignalSpy errors(&client, &WikipediaSummaryClient::errorOccurred);
        client.fetchSummary("Slow");
        QTRY_COMPARE_WITH_TIMEOUT(errors.count(), 1, WikipediaSummaryClient::RequestTimeoutMs + 1000);
        QCOMPARE(m_network.pendingReplyCount(), 0);
    }

    void unicodeTitleIsEncodedAsOnePathSegment() {
        const QString title = QStringLiteral("São Paulo/100%");
        const QUrl url(QStringLiteral("https://en.wikipedia.org/api/rest_v1/page/summary/S%C3%A3o%20Paulo%2F100%25"));
        m_network.addFixture("GET", url, R"JSON({"title":"São Paulo/100%","pageid":4,"extract":"Intro"})JSON");
        WikipediaSummaryClient client;
        QSignalSpy received(&client, &WikipediaSummaryClient::summaryReceived);
        client.fetchSummary(title);
        QTRY_COMPARE(received.count(), 1);
        QCOMPARE(m_network.requestCount(), 1);
    }

    void obsoleteReplyCannotEmitItsError() {
        const QUrl oldUrl(QStringLiteral("https://en.wikipedia.org/api/rest_v1/page/summary/Obsolete"));
        m_network.addFixture("GET", oldUrl, "failure", "text/plain", 503);
        m_network.deferNextReply(true);
        WikipediaSummaryClient client;
        QSignalSpy errors(&client, &WikipediaSummaryClient::errorOccurred);
        client.fetchSummary("Obsolete");
        QTRY_COMPARE(m_network.pendingReplyCount(), 1);
        client.cancelPendingRequests();
        m_network.completeNextReply();
        QTest::qWait(10);
        QCOMPARE(errors.count(), 0);
    }
};
QTEST_GUILESS_MAIN(WikipediaSummaryClientTest)
#include "WikipediaSummaryClientTest.moc"

