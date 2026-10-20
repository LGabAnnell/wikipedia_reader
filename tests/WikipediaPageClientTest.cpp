// tests/WikipediaPageClientTest.cpp
#include <QtTest/QtTest>
#include <QByteArray>
#include "wikipedia_page_client.h"
#include "wikipedia_network_access_manager.h"
#include "support/FakeNetworkAccessManager.h"

class WikipediaPageClientTest : public QObject {
    Q_OBJECT

  private:
    NetworkFixtureController m_network;

    const QUrl m_pageUrl{"https://en.wikipedia.org/w/api.php?action=query&format=json&prop=extracts%7Cimages&pageids=736&explaintext=1&imlimit=50"};

  private slots:
    void initTestCase() {
        connect(&m_network, &NetworkFixtureController::unexpectedRequest, this,
                [](const QString &request) { qFatal("%s", qPrintable(request)); });
        WikipediaNetwork::installNetworkAccessManagerFactory(&m_network);
    }

    void cleanupTestCase() { WikipediaNetwork::installNetworkAccessManagerFactory(nullptr); }
    void init() { m_network.clearRequests(); }
    void cleanup() { QCOMPARE(m_network.pendingReplyCount(), 0); }

    void testGetPageByIdMissingPage_data() {
        QTest::addColumn<QByteArray>("response");
        QTest::newRow("empty pages") << QByteArray(R"({"query":{"pages":{}}})");
        QTest::newRow("different page") << QByteArray(R"({"query":{"pages":{"999":{"pageid":999,"title":"Other"}}}})");
        QTest::newRow("missing page") << QByteArray(R"({"query":{"pages":{"736":{"pageid":736,"title":"Deleted","missing":""}}}})");
        QTest::newRow("invalid page") << QByteArray(R"({"query":{"pages":{"736":{"pageid":736,"title":"Invalid","invalid":""}}}})");
        QTest::newRow("missing title") << QByteArray(R"({"query":{"pages":{"736":{"pageid":736}}}})");
        QTest::newRow("missing query") << QByteArray("{}");
        QTest::newRow("malformed json") << QByteArray("not json");
    }

    void testGetPageByIdMissingPage() {
        QFETCH(QByteArray, response);
        m_network.addFixture("GET", m_pageUrl, response);
        WikipediaPageClient client;
        QSignalSpy pages(&client, &WikipediaPageClient::pageReceived);
        QSignalSpy errors(&client, &WikipediaPageClient::errorOccurred);
        m_network.deferNextReply();

        client.getPageById(736);
        QCOMPARE(pages.count(), 0);
        QCOMPARE(errors.count(), 0);
        m_network.completeNextReply();

        QTRY_COMPARE(errors.count(), 1);
        QVERIFY(!errors.first().first().toString().isEmpty());
        QCOMPARE(pages.count(), 0);
        QCOMPARE(m_network.requestCount(), 1);
    }

    void testGetPageByIdSuccess() {
        m_network.addFixture("GET", m_pageUrl,
            R"({"query":{"pages":{"736":{"pageid":736,"title":"Albert Einstein","extract":"Summary"}}}})");
        const QUrl htmlUrl("https://en.wikipedia.org/w/api.php?action=parse&format=json&prop=text&disableeditsection=true&formatversion=2&pageid=736");
        m_network.addFixture("GET", htmlUrl, R"({"parse":{"text":"<p>A theoretical physicist.</p>"}})");
        WikipediaPageClient client;
        QSignalSpy pages(&client, &WikipediaPageClient::pageReceived);
        QSignalSpy errors(&client, &WikipediaPageClient::errorOccurred);

        client.getPageById(736);

        QTRY_COMPARE(pages.count(), 1);
        const page result = pages.first().first().value<page>();
        QCOMPARE(result.pageid, 736);
        QCOMPARE(result.title, QString("Albert Einstein"));
        QVERIFY(result.extract.contains("A theoretical physicist."));
        QCOMPARE(errors.count(), 0);
        QCOMPARE(m_network.requestCount(), 2);
    }

    void testParsePage() {
        QByteArray json = R"({"query":{"pages":{"736":{
            "title":"Albert Einstein",
            "extract":"A theoretical physicist.",
            "pageid":736
        }}}})";

        page p = WikipediaPageClient::parsePage(json, "Albert Einstein");
        QCOMPARE(p.title, QString("Albert Einstein"));
        QCOMPARE(p.extract, QString("A theoretical physicist."));
        QCOMPARE(p.pageid, 736);
        QVERIFY(p.imageUrls.isEmpty());
    }

    void testParsePageTitleMismatch() {
        QByteArray json = R"({"query":{"pages":{"736":{
            "title":"Albert Einstein",
            "extract":"A theoretical physicist.",
            "pageid":736
        }}}})";

        page p = WikipediaPageClient::parsePage(json, "Nonexistent");
        QVERIFY(p.title.isEmpty());
        QVERIFY(p.extract.isEmpty());
    }

    void testParsePageMalformedJson() {
        QByteArray json = "not json";
        page p = WikipediaPageClient::parsePage(json, "Anything");
        QVERIFY(p.title.isEmpty());
    }

    void testParsePageWithImages() {
        QByteArray json = R"({"query":{"pages":{"736":{
            "title":"Albert Einstein",
            "extract":"A theoretical physicist.",
            "pageid":736,
            "images":[
                {"title":"File:Einstein.jpg"},
                {"title":"File:Einstein2.jpg"}
            ]
        }}}})";

        page p = WikipediaPageClient::parsePageWithImages(json, 736);
        QCOMPARE(p.title, QString("Albert Einstein"));
        QCOMPARE(p.extract, QString("A theoretical physicist."));
        QCOMPARE(p.pageid, 736);
        // parsePageWithImages stores image titles in imageUrls temporarily
        QCOMPARE(p.imageUrls.size(), 2);
        QCOMPARE(p.imageUrls[0], QString("File:Einstein.jpg"));
        QCOMPARE(p.imageUrls[1], QString("File:Einstein2.jpg"));
    }

    void testParsePageWithImagesNoImages() {
        QByteArray json = R"({"query":{"pages":{"736":{
            "title":"Albert Einstein",
            "extract":"A theoretical physicist.",
            "pageid":736
        }}}})";

        page p = WikipediaPageClient::parsePageWithImages(json, 736);
        QCOMPARE(p.title, QString("Albert Einstein"));
        QVERIFY(p.imageUrls.isEmpty());
    }

    void testParsePageWithImagesPageIdMismatch() {
        QByteArray json = R"({"query":{"pages":{"736":{
            "title":"Albert Einstein",
            "extract":"A theoretical physicist.",
            "pageid":736
        }}}})";

        page p = WikipediaPageClient::parsePageWithImages(json, 999);
        QVERIFY(p.title.isEmpty());
    }

    void testParseSections() {
        QByteArray json = R"({"parse":{"tocdata":{"sections":[
            {"line":"History","tocLevel":1,"anchor":"History","index":"1"},
            {"line":"Early life","tocLevel":2,"anchor":"Early_life","index":"2"}
        ]}}})";

        QVector<section> sections = WikipediaPageClient::parseSections(json);
        QCOMPARE(sections.size(), 2);
        QCOMPARE(sections[0].title, QString("History"));
        QCOMPARE(sections[0].level, 1);
        QCOMPARE(sections[0].anchor, QString("History"));
        QCOMPARE(sections[0].index, 1);
        QCOMPARE(sections[1].title, QString("Early life"));
        QCOMPARE(sections[1].level, 2);
        QCOMPARE(sections[1].anchor, QString("Early_life"));
        QCOMPARE(sections[1].index, 2);
    }

    static void testParseSectionsEmpty() {
        QByteArray json = R"({"parse":{"tocdata":{"sections":[]}}})";
        QVector<section> sections = WikipediaPageClient::parseSections(json);
        QVERIFY(sections.isEmpty());
    }

    static void testParseSectionsMissingParse() {
        QByteArray json = R"({})";
        QVector<section> sections = WikipediaPageClient::parseSections(json);
        QVERIFY(sections.isEmpty());
    }

    static void testParseSectionsMalformedJson() {
        QByteArray json = "not json";
        QVector<section> sections = WikipediaPageClient::parseSections(json);
        QVERIFY(sections.isEmpty());
    }
};

QTEST_MAIN(WikipediaPageClientTest)
#include "WikipediaPageClientTest.moc"
