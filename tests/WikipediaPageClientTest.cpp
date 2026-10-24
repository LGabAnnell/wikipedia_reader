// tests/WikipediaPageClientTest.cpp
#include <QtTest/QtTest>
#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "wikipedia_page_client.h"
#include "wikipedia_network_access_manager.h"
#include "support/FakeNetworkAccessManager.h"

class WikipediaPageClientTest : public QObject {
    Q_OBJECT

  private:
    NetworkFixtureController m_network;

    const QUrl m_pageUrl{"https://en.wikipedia.org/w/api.php?action=query&format=json&pageids=736"};

  private slots:
    void initTestCase() {
        connect(&m_network, &NetworkFixtureController::unexpectedRequest, this,
                [](const QString &request) { qFatal("%s", qPrintable(request)); });
        WikipediaNetwork::installNetworkAccessManagerFactory(&m_network);
        QVERIFY(metaObject()->indexOfSlot("testParseSectionsMalformedJson()") >= 0);
    }

    void cleanupTestCase() { WikipediaNetwork::installNetworkAccessManagerFactory(nullptr); }
    void init() { m_network.clearRequests(); }
    void cleanup() { QCOMPARE(m_network.pendingReplyCount(), 0); }

    void testResolveTitleWithoutPageId_data() {
        QTest::addColumn<QByteArray>("response");
        QTest::addColumn<int>("status");
        QTest::newRow("missing title") << QByteArray(R"({"query":{"pages":{"-1":{"missing":"","title":"Some title"}}}})") << 200;
        QTest::newRow("zero page id") << QByteArray(R"({"query":{"pages":{"0":{"pageid":0,"title":"Some title"}}}})") << 200;
        QTest::newRow("negative page id") << QByteArray(R"({"query":{"pages":{"-2":{"pageid":-2,"title":"Some title"}}}})") << 200;
        QTest::newRow("empty pages") << QByteArray(R"({"query":{"pages":{}}})") << 200;
        QTest::newRow("missing query") << QByteArray("{}") << 200;
        QTest::newRow("api error") << QByteArray(R"({"error":{"code":"badtitle"}})") << 200;
        QTest::newRow("network error") << QByteArray("failure") << 500;
        QTest::newRow("malformed json") << QByteArray("not json") << 200;
    }

    void testResolveTitleWithoutPageId() {
        QFETCH(QByteArray, response);
        QFETCH(int, status);
        const QUrl resolveUrl("https://en.wikipedia.org/w/api.php?action=query&format=json&titles=Some%20title&prop=pageprops%7Cpageids");
        m_network.addFixture("GET", resolveUrl, response, "application/json", status);
        WikipediaPageClient client;
        QSignalSpy resolved(&client, &WikipediaPageClient::pageIdResolved);
        QSignalSpy errors(&client, &WikipediaPageClient::errorOccurred);
        m_network.deferNextReply();

        client.resolveTitleToPageId("Some title");
        QCOMPARE(errors.count(), 0);
        m_network.completeNextReply();

        QTRY_COMPARE(errors.count(), 1);
        QVERIFY(!errors.first().first().toString().isEmpty());
        QCOMPARE(resolved.count(), 0);
        QCOMPARE(m_network.requestCount(), 1);
    }

    void testResolveTitleSuccessfully() {
        const QUrl resolveUrl("https://en.wikipedia.org/w/api.php?action=query&format=json&titles=Some%20title&prop=pageprops%7Cpageids");
        m_network.addFixture("GET", resolveUrl,
            R"({"query":{"pages":{"123":{"pageid":123,"title":"Some title"}}}})");
        WikipediaPageClient client;
        QSignalSpy resolved(&client, &WikipediaPageClient::pageIdResolved);
        QSignalSpy errors(&client, &WikipediaPageClient::errorOccurred);

        client.resolveTitleToPageId("Some title");

        QTRY_COMPARE(resolved.count(), 1);
        QCOMPARE(resolved.first().first().toInt(), 123);
        QCOMPARE(errors.count(), 0);
        QCOMPARE(m_network.requestCount(), 1);
    }

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

    void testGetPageByIdSuccess_data() {
        QTest::addColumn<QString>("language");
        QTest::newRow("English") << QString("en");
        QTest::newRow("French") << QString("fr");
    }

    void testGetPageByIdSuccess() {
        QFETCH(QString, language);
        const QString apiUrl = "https://" + language + ".wikipedia.org/w/api.php";
        // Article loading only needs page identity here; HTML is fetched separately.
        m_network.addFixture("GET", QUrl(apiUrl + "?action=query&format=json&pageids=736"),
            R"({"query":{"pages":{"736":{"pageid":736,"title":"Albert Einstein"}}}})");
        const QUrl htmlUrl(apiUrl + "?action=parse&format=json&prop=text&disableeditsection=true&formatversion=2&pageid=736");
        m_network.addFixture("GET", htmlUrl, R"({"parse":{"text":"<p>A theoretical physicist.</p>"}})");
        WikipediaPageClient client;
        client.setLanguage(language);
        QSignalSpy pages(&client, &WikipediaPageClient::pageReceived);
        QSignalSpy errors(&client, &WikipediaPageClient::errorOccurred);

        client.getPageById(736);

        QTRY_COMPARE(pages.count(), 1);
        const page result = pages.first().first().value<page>();
        QCOMPARE(result.pageid, 736);
        QCOMPARE(result.title, QString("Albert Einstein"));
        QVERIFY(result.extract.contains("A theoretical physicist."));
        QVERIFY(result.imageUrls.isEmpty());
        QVERIFY(result.imageTitles.isEmpty());
        QCOMPARE(errors.count(), 0);
        QCOMPARE(m_network.requestCount(), 2);
    }

    void testGetPageByIdRejectsInvalidParseResponse_data() {
        QTest::addColumn<QByteArray>("response");
        QTest::addColumn<QString>("errorText");
        QTest::addColumn<QString>("errorInfo");
        QTest::newRow("malformed JSON") << QByteArray("not json") << QString() << QString();
        QTest::newRow("non-object JSON") << QByteArray("[]") << QString() << QString();
        QTest::newRow("API error")
            << QByteArray("{\"error\":{\"code\":\"badtoken\",\"info\":\"Invalid token\"},"
                          "\"parse\":{\"text\":\"<img class=\\\"mwe-math-fallback-image-inline\\\" "
                          "src=\\\"https://math.example/formula.svg\\\">\"}}")
            << QString("badtoken") << QString("Invalid token");
        QTest::newRow("missing parse object") << QByteArray(R"({})") << QString() << QString();
        QTest::newRow("non-object parse") << QByteArray(R"({"parse":"invalid"})") << QString() << QString();
        QTest::newRow("missing text") << QByteArray(R"({"parse":{}})") << QString() << QString();
        QTest::newRow("null text") << QByteArray(R"({"parse":{"text":null}})") << QString() << QString();
        QTest::newRow("non-string text") << QByteArray(R"({"parse":{"text":42}})") << QString() << QString();
    }

    void testGetPageByIdRejectsInvalidParseResponse() {
        QFETCH(QByteArray, response);
        QFETCH(QString, errorText);
        QFETCH(QString, errorInfo);
        const QUrl parseUrl("https://en.wikipedia.org/w/api.php?action=parse&format=json&prop=text&disableeditsection=true&formatversion=2&pageid=736");
        m_network.addFixture("GET", m_pageUrl,
            R"({"query":{"pages":{"736":{"pageid":736,"title":"Albert Einstein"}}}})");
        m_network.addFixture("GET", parseUrl, response);
        WikipediaPageClient client;
        QSignalSpy pages(&client, &WikipediaPageClient::pageReceived);
        QSignalSpy errors(&client, &WikipediaPageClient::errorOccurred);

        client.getPageById(736);

        QTRY_COMPARE(errors.count(), 1);
        QVERIFY(!errors.first().first().toString().isEmpty());
        if (!errorText.isEmpty())
            QVERIFY(errors.first().first().toString().contains(errorText));
        if (!errorInfo.isEmpty())
            QVERIFY(errors.first().first().toString().contains(errorInfo));
        QCOMPARE(pages.count(), 0);
        QCOMPARE(m_network.requestCount(), 2);
    }

    void testGetPageByIdAcceptsEmptyParseText() {
        const QUrl parseUrl("https://en.wikipedia.org/w/api.php?action=parse&format=json&prop=text&disableeditsection=true&formatversion=2&pageid=736");
        m_network.addFixture("GET", m_pageUrl,
            R"({"query":{"pages":{"736":{"pageid":736,"title":"Albert Einstein"}}}})");
        m_network.addFixture("GET", parseUrl, R"({"parse":{"text":""}})");
        WikipediaPageClient client;
        QSignalSpy pages(&client, &WikipediaPageClient::pageReceived);
        QSignalSpy errors(&client, &WikipediaPageClient::errorOccurred);

        client.getPageById(736);

        QTRY_COMPARE(pages.count(), 1);
        QVERIFY(pages.first().first().value<page>().extract.isEmpty());
        QCOMPARE(errors.count(), 0);
        QCOMPARE(m_network.requestCount(), 2);
    }

    void testFormulaWorkAndCacheBudgetsAcrossRepeatedLoads() {
        QString html;
        constexpr int formulaCount = 257;
        const QByteArray svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"2\" height=\"2\"></svg>";
        for (int i = 0; i < formulaCount; ++i) {
            const QString url = QString("https://math.example/formula-%1.svg").arg(i);
            html += QString("<img class=\"mwe-math-fallback-image-inline\" src=\"%1\">").arg(url);
            m_network.addFixture("GET", QUrl(url), svg, "image/svg+xml");
        }
        m_network.addFixture("GET", m_pageUrl,
            R"({"query":{"pages":{"736":{"pageid":736,"title":"Formula article"}}}})");
        const QUrl parseUrl("https://en.wikipedia.org/w/api.php?action=parse&format=json&prop=text&disableeditsection=true&formatversion=2&pageid=736");
        m_network.addFixture("GET", parseUrl,
            QJsonDocument(QJsonObject{{"parse", QJsonObject{{"text", html}}}}).toJson());

        WikipediaPageClient client;
        QSignalSpy pages(&client, &WikipediaPageClient::pageReceived);
        client.getPageById(736);
        QTRY_COMPARE_WITH_TIMEOUT(pages.count(), 1, 10'000);
        QCOMPARE(m_network.requestCount(), 130); // page, HTML, and the 128-source per-load cap
        QCOMPARE(client.mathImageCacheEntryCount(), 128);
        QVERIFY(client.mathImageCachePayloadBytes() <= 16 * 1024 * 1024);
        qInfo() << "Formula cache after load 1:" << client.mathImageCacheEntryCount()
                << "entries," << client.mathImageCachePayloadBytes() << "payload bytes; pending replies:"
                << m_network.pendingReplyCount();

        client.getPageById(736);
        QTRY_COMPARE_WITH_TIMEOUT(pages.count(), 2, 10'000);
        QCOMPARE(m_network.requestCount(), 260); // The next 128 uncached formulas are fetched.
        QCOMPARE(client.mathImageCacheEntryCount(), 256);
        QVERIFY(client.mathImageCachePayloadBytes() <= 16 * 1024 * 1024);
        qInfo() << "Formula cache after load 2:" << client.mathImageCacheEntryCount()
                << "entries," << client.mathImageCachePayloadBytes() << "payload bytes; pending replies:"
                << m_network.pendingReplyCount();

        client.getPageById(736);
        QTRY_COMPARE_WITH_TIMEOUT(pages.count(), 3, 10'000);
        QCOMPARE(m_network.requestCount(), 263); // Formula 256 is fetched and evicts least-recently used 0.
        QCOMPARE(client.mathImageCacheEntryCount(), 256);
        qInfo() << "Formula cache after load 3:" << client.mathImageCacheEntryCount()
                << "entries," << client.mathImageCachePayloadBytes() << "payload bytes; pending replies:"
                << m_network.pendingReplyCount();

        client.getPageById(736);
        QTRY_COMPARE_WITH_TIMEOUT(pages.count(), 4, 10'000);
        QCOMPARE(m_network.requestCount(), 266); // Formula 0 was evicted and must be fetched again.
        QCOMPARE(client.mathImageCacheEntryCount(), 256);
        QVERIFY(client.mathImageCachePayloadBytes() <= 16 * 1024 * 1024);
        qInfo() << "Formula cache after load 4:" << client.mathImageCacheEntryCount()
                << "entries," << client.mathImageCachePayloadBytes() << "payload bytes; pending replies:"
                << m_network.pendingReplyCount();
    }

    void testFormulaCacheEvictsByPayloadBytes() {
        const QByteArray svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"2\" height=\"2\"></svg>";
        const QUrl parseUrl("https://en.wikipedia.org/w/api.php?action=parse&format=json&prop=text&disableeditsection=true&formatversion=2&pageid=736");
        const QString oneFormulaUrl = QStringLiteral("https://math.example/byte-probe.svg");
        m_network.addFixture("GET", m_pageUrl,
            R"({"query":{"pages":{"736":{"pageid":736,"title":"Formula byte probe"}}}})");
        m_network.addFixture("GET", parseUrl,
            QJsonDocument(QJsonObject{{"parse", QJsonObject{{"text", QStringLiteral("<img class=\"mwe-math-fallback-image-inline\" src=\"%1\">").arg(oneFormulaUrl)}}}}).toJson());
        m_network.addFixture("GET", QUrl(oneFormulaUrl), svg, "image/svg+xml");
        WikipediaPageClient probe;
        QSignalSpy probePages(&probe, &WikipediaPageClient::pageReceived);
        probe.getPageById(736);
        QTRY_COMPARE_WITH_TIMEOUT(probePages.count(), 1, 1000);
        const qsizetype oneEntryBytes = probe.mathImageCachePayloadBytes();
        QVERIFY(oneEntryBytes > 0);

        m_network.clearRequests();
        QString html;
        constexpr int formulaCount = 8;
        for (int i = 0; i < formulaCount; ++i) {
            const QString url = QString("https://math.example/byte-budget-%1.svg").arg(i);
            html += QString("<img class=\"mwe-math-fallback-image-inline\" src=\"%1\">").arg(url);
            m_network.addFixture("GET", QUrl(url), svg, "image/svg+xml");
        }
        m_network.addFixture("GET", parseUrl,
            QJsonDocument(QJsonObject{{"parse", QJsonObject{{"text", html}}}}).toJson());
        WikipediaPageClientLimits limits;
        limits.formulaCacheEntries = 64;
        limits.formulaCacheBytes = oneEntryBytes * 3 + 1;
        WikipediaPageClient client(nullptr, limits);
        QSignalSpy pages(&client, &WikipediaPageClient::pageReceived);
        client.getPageById(736);
        QTRY_COMPARE_WITH_TIMEOUT(pages.count(), 1, 2000);
        QCOMPARE(m_network.requestCount(), formulaCount + 2);
        QCOMPARE(client.mathImageCacheEntryCount(), 3);
        QCOMPARE(client.mathImageCachePayloadBytes(), oneEntryBytes * 3);
        QVERIFY(client.mathImageCachePayloadBytes() <= limits.formulaCacheBytes);
        qInfo() << "Formula byte-limit eviction:" << client.mathImageCacheEntryCount()
                << "entries," << client.mathImageCachePayloadBytes() << "of" << limits.formulaCacheBytes
                << "payload bytes; pending replies:" << m_network.pendingReplyCount();
    }

    void testFormulaEnrichmentDeadlinePublishesArticleFallback() {
        const QUrl parseUrl("https://en.wikipedia.org/w/api.php?action=parse&format=json&prop=text&disableeditsection=true&formatversion=2&pageid=736");
        m_network.addFixture("GET", m_pageUrl,
            R"({"query":{"pages":{"736":{"pageid":736,"title":"Formula article"}}}})");
        const QString stalledHtml = QStringLiteral("<p>Text<img class=\"mwe-math-fallback-image-inline\" src=\"https://math.example/stalled.svg\"></p>");
        const QByteArray parseResponse = QJsonDocument(QJsonObject{{"parse", QJsonObject{{"text", stalledHtml}}}}).toJson();
        m_network.addFixture("GET", parseUrl, parseResponse);
        m_network.addFixture("GET", QUrl("https://math.example/stalled.svg"), "<svg/>", "image/svg+xml");
        WikipediaPageClientLimits limits;
        limits.formulaEnrichmentTimeoutMs = 40;
        WikipediaPageClient client(nullptr, limits);
        QSignalSpy pages(&client, &WikipediaPageClient::pageReceived);
        QSignalSpy errors(&client, &WikipediaPageClient::errorOccurred);

        m_network.deferNextReply(true);
        client.getPageById(736);
        m_network.deferNextReply(true);
        m_network.completeNextReply();
        QCOMPARE(m_network.pendingReplyCount(), 1); // HTML stage
        m_network.deferNextReply(true);
        m_network.completeNextReply();
        QCOMPARE(m_network.pendingReplyCount(), 1); // Optional formula stage

        QTRY_COMPARE_WITH_TIMEOUT(pages.count(), 1, 1000);
        QVERIFY(pages.first().first().value<page>().extract.contains("stalled.svg"));
        QCOMPARE(errors.count(), 0);
        m_network.completeNextReply(); // Late optional response must not republish the article.
        QTest::qWait(25);
        QCOMPARE(pages.count(), 1);
        QCOMPARE(m_network.pendingReplyCount(), 0);
    }

    void testFormulaFanoutStopsAtTwoAndCanceledWorkDoesNotContinue() {
        const QUrl parseUrl("https://en.wikipedia.org/w/api.php?action=parse&format=json&prop=text&disableeditsection=true&formatversion=2&pageid=736");
        m_network.addFixture("GET", m_pageUrl,
            R"({"query":{"pages":{"736":{"pageid":736,"title":"Canceled formula article"}}}})");
        QString html;
        constexpr int formulaCount = 12;
        for (int i = 0; i < formulaCount; ++i) {
            const QString url = QString("https://math.example/canceled-%1.svg").arg(i);
            html += QString("<img class=\"mwe-math-fallback-image-inline\" src=\"%1\">").arg(url);
            m_network.addFixture("GET", QUrl(url), "<svg/>", "image/svg+xml");
        }
        m_network.addFixture("GET", parseUrl,
            QJsonDocument(QJsonObject{{"parse", QJsonObject{{"text", html}}}}).toJson());

        WikipediaPageClientLimits limits;
        limits.formulaEnrichmentTimeoutMs = 60;
        WikipediaPageClient client(nullptr, limits);
        QSignalSpy pages(&client, &WikipediaPageClient::pageReceived);
        QSignalSpy errors(&client, &WikipediaPageClient::errorOccurred);

        m_network.deferNextReply(true);
        client.getPageById(736);
        m_network.deferNextReply(true);
        m_network.completeNextReply(); // Page metadata starts the HTML request.
        QCOMPARE(m_network.pendingReplyCount(), 1);
        m_network.deferNextReply(true);
        m_network.completeNextReply(); // HTML starts exactly two formula requests.
        QCOMPARE(m_network.requestCount(), 4);
        QCOMPARE(m_network.pendingReplyCount(), 1);

        client.cancelPendingRequests();
        m_network.completeNextReply(); // A response allowed to finish after abort must remain stale.
        QTRY_COMPARE_WITH_TIMEOUT(m_network.pendingReplyCount(), 0, 1000);
        QTest::qWait(100);
        QCOMPARE(m_network.requestCount(), 4);
        QCOMPARE(pages.count(), 0);
        QCOMPARE(errors.count(), 0);
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
        QCOMPARE(p.imageTitles, QStringList({"File:Einstein.jpg", "File:Einstein2.jpg"}));
        QVERIFY(p.imageUrls.isEmpty());
        QVERIFY(p.imageDescriptions.isEmpty());
    }

    void testGalleryPreservesRequestedImageOrder() {
        m_network.addFixture("GET", QUrl("https://en.wikipedia.org/w/api.php?action=query&format=json"
            "&prop=extracts|images&pageids=736&explaintext=1&imlimit=50"),
            "{\"query\":{\"pages\":{\"736\":{"
            "\"pageid\":736,\"title\":\"Gallery article\",\"images\":["
            "{\"title\":\"File:Zebra.jpg\"},"
            "{\"title\":\"File:Missing.jpg\"},"
            "{\"title\":\"File:Alpha.jpg\"},"
            "{\"title\":\"File:NoInfo.jpg\"},"
            "{\"title\":\"File:EmptyInfo.jpg\"},"
            "{\"title\":\"File:NoUrl.jpg\"},"
            "{\"title\":\"File:Original.jpg\"}"
            "]"
            "}}}}");
        m_network.addFixture("GET", QUrl("https://en.wikipedia.org/w/api.php?action=query&format=json"
            "&prop=imageinfo&iiprop=url|extmetadata|mime&iiurlheight=330&iiurlwidth=330"
            "&titles=File:Zebra.jpg|File:Missing.jpg|File:Alpha.jpg|File:NoInfo.jpg"
            "|File:EmptyInfo.jpg|File:NoUrl.jpg|File:Original.jpg"),
            "{\"query\":{\"pages\":{"
            "\"1\":{\"title\":\"File:Alpha.jpg\",\"imageinfo\":[{\"thumburl\":\"https://example.org/alpha-thumb.jpg\"}]},"
            "\"10\":{\"title\":\"File:EmptyInfo.jpg\",\"imageinfo\":[]},"
            "\"2\":{\"title\":\"File:Zebra.jpg\",\"imageinfo\":[{"
            "\"thumburl\":\"https://example.org/zebra-thumb.jpg\",\"url\":\"https://example.org/zebra.jpg\","
            "\"extmetadata\":{\"ImageDescription\":{\"value\":\"<p>Zebra &amp; stripes</p>\"}}"
            "}]},"
            "\"3\":{\"title\":\"File:NoInfo.jpg\",\"missing\":\"\"},"
            "\"4\":{\"title\":\"File:NoUrl.jpg\",\"imageinfo\":[{}]},"
            "\"5\":{\"title\":\"File:Original.jpg\",\"imageinfo\":[{"
            "\"url\":\"https://example.org/original.jpg\","
            "\"extmetadata\":{\"ImageDescription\":{\"value\":\"Original caption\"}}"
            "}]}"
            "}}}");
        WikipediaPageClient client;
        QSignalSpy pages(&client, &WikipediaPageClient::pageWithImagesReceived);
        QSignalSpy errors(&client, &WikipediaPageClient::errorOccurred);
        m_network.deferNextReply();

        client.getPageWithImages(736);
        QCOMPARE(pages.count(), 0);
        m_network.completeNextReply();

        QTRY_COMPARE(pages.count(), 1);
        const page result = pages.first().first().value<page>();
        QCOMPARE(result.pageid, 736);
        QCOMPARE(result.title, QString("Gallery article"));
        QCOMPARE(result.imageTitles, QStringList({"File:Zebra.jpg", "File:Alpha.jpg", "File:Original.jpg"}));
        QCOMPARE(result.imageUrls, QStringList({"https://example.org/zebra-thumb.jpg",
                                              "https://example.org/alpha-thumb.jpg",
                                              "https://example.org/original.jpg"}));
        QCOMPARE(result.imageDescriptions, QStringList({"Zebra & stripes", "", "Original caption"}));
        QCOMPARE(errors.count(), 0);
        QCOMPARE(m_network.requestCount(), 2);
    }

    void testGalleryRejectsInvalidPageResponse_data() {
        QTest::addColumn<QByteArray>("response");
        QTest::newRow("malformed json") << QByteArray("not json");
        QTest::newRow("non-object json") << QByteArray("[]");
        QTest::newRow("api error") << QByteArray(R"({"error":{"code":"badvalue","info":"Invalid page"}})");
        QTest::newRow("empty api error with otherwise valid page") << QByteArray(
            R"({"error":{},"query":{"pages":{"736":{"pageid":736,"title":"Valid"}}}})");
        QTest::newRow("missing query") << QByteArray("{}");
        QTest::newRow("wrong pages type") << QByteArray(R"({"query":{"pages":[]}})");
        QTest::newRow("empty pages") << QByteArray(R"({"query":{"pages":{}}})");
        QTest::newRow("different page id") << QByteArray(R"({"query":{"pages":{"999":{"pageid":999,"title":"Other"}}}})");
        QTest::newRow("missing marker") << QByteArray(R"({"query":{"pages":{"736":{"pageid":736,"title":"Missing","missing":""}}}})");
        QTest::newRow("invalid marker") << QByteArray(R"({"query":{"pages":{"736":{"pageid":736,"title":"Invalid","invalid":""}}}})");
        QTest::newRow("missing title") << QByteArray(R"({"query":{"pages":{"736":{"pageid":736}}}})");
        QTest::newRow("blank title") << QByteArray(R"({"query":{"pages":{"736":{"pageid":736,"title":"  "}}}})");
    }

    void testGalleryRejectsInvalidPageResponse() {
        QFETCH(QByteArray, response);
        m_network.addFixture("GET", QUrl("https://en.wikipedia.org/w/api.php?action=query&format=json"
            "&prop=extracts|images&pageids=736&explaintext=1&imlimit=50"), response);
        WikipediaPageClient client;
        QSignalSpy pages(&client, &WikipediaPageClient::pageWithImagesReceived);
        QSignalSpy errors(&client, &WikipediaPageClient::errorOccurred);
        m_network.deferNextReply();

        client.getPageWithImages(736);
        m_network.completeNextReply();

        QTRY_COMPARE(errors.count(), 1);
        QVERIFY(!errors.first().first().toString().trimmed().isEmpty());
        QCOMPARE(pages.count(), 0);
        QCOMPARE(m_network.requestCount(), 1);
    }

    void testGalleryWithNoImagesSucceeds() {
        m_network.addFixture("GET", QUrl("https://en.wikipedia.org/w/api.php?action=query&format=json"
            "&prop=extracts|images&pageids=736&explaintext=1&imlimit=50"),
            R"({"query":{"pages":{"736":{"pageid":736,"title":"Empty gallery","extract":"","images":[]}}}})");
        WikipediaPageClient client;
        QSignalSpy pages(&client, &WikipediaPageClient::pageWithImagesReceived);
        QSignalSpy errors(&client, &WikipediaPageClient::errorOccurred);

        client.getPageWithImages(736);

        QTRY_COMPARE(pages.count(), 1);
        const page result = pages.first().first().value<page>();
        QCOMPARE(result.pageid, 736);
        QCOMPARE(result.title, QString("Empty gallery"));
        QVERIFY(result.imageUrls.isEmpty());
        QCOMPARE(errors.count(), 0);
        QCOMPARE(m_network.requestCount(), 1);
    }

    void testGalleryWithOmittedImagesSucceeds() {
        m_network.addFixture("GET", QUrl("https://en.wikipedia.org/w/api.php?action=query&format=json"
            "&prop=extracts|images&pageids=736&explaintext=1&imlimit=50"),
            R"({"query":{"pages":{"736":{"pageid":736,"title":"Gallery without image list"}}}})");
        WikipediaPageClient client;
        QSignalSpy pages(&client, &WikipediaPageClient::pageWithImagesReceived);
        QSignalSpy errors(&client, &WikipediaPageClient::errorOccurred);

        client.getPageWithImages(736);

        QTRY_COMPARE(pages.count(), 1);
        QCOMPARE(pages.first().first().value<page>().title, QString("Gallery without image list"));
        QCOMPARE(errors.count(), 0);
        QCOMPARE(m_network.requestCount(), 1);
    }

    void testGalleryFollowsContinuationAcrossPages() {
        QUrl firstUrl("https://en.wikipedia.org/w/api.php");
        QUrlQuery firstQuery;
        firstQuery.addQueryItem("action", "query");
        firstQuery.addQueryItem("format", "json");
        firstQuery.addQueryItem("prop", "extracts|images");
        firstQuery.addQueryItem("pageids", "736");
        firstQuery.addQueryItem("explaintext", "1");
        firstQuery.addQueryItem("imlimit", "50");
        firstUrl.setQuery(firstQuery);
        m_network.addFixture("GET", firstUrl,
            R"({"continue":{"continue":"||","imcontinue":"736|B"},"query":{"pages":{"736":{"pageid":736,"title":"Gallery article","images":[{"title":"File:A.jpg"}]}}}})");
        QUrl secondUrl("https://en.wikipedia.org/w/api.php");
        firstQuery.addQueryItem("continue", "||");
        firstQuery.addQueryItem("imcontinue", "736|B");
        secondUrl.setQuery(firstQuery);
        m_network.addFixture("GET", secondUrl,
            R"({"query":{"pages":{"736":{"pageid":736,"title":"Gallery article","images":[{"title":"File:B.jpg"}]}}}})");
        m_network.addFixture("GET", QUrl("https://en.wikipedia.org/w/api.php?action=query&format=json"
            "&prop=imageinfo&iiprop=url|extmetadata|mime&iiurlheight=330&iiurlwidth=330&titles=File:A.jpg"),
            R"({"query":{"pages":{"1":{"title":"File:A.jpg","imageinfo":[{"url":"https:\/\/img.example/a.jpg"}]}}}})");
        m_network.addFixture("GET", QUrl("https://en.wikipedia.org/w/api.php?action=query&format=json"
            "&prop=imageinfo&iiprop=url|extmetadata|mime&iiurlheight=330&iiurlwidth=330&titles=File:B.jpg"),
            R"({"query":{"pages":{"2":{"title":"File:B.jpg","imageinfo":[{"url":"https:\/\/img.example/b.jpg"}]}}}})");

        WikipediaPageClient client;
        QSignalSpy pages(&client, &WikipediaPageClient::pageWithImagesReceived);
        client.getPageWithImages(736);
        QTRY_COMPARE(pages.count(), 1);
        const page first = pages.takeFirst().first().value<page>();
        QCOMPARE(first.imageContinuation.value("imcontinue").toString(), QString("736|B"));
        client.getPageWithImages(736, first.imageContinuation);
        QTRY_COMPARE(pages.count(), 1);
        const page second = pages.first().first().value<page>();
        QCOMPARE(second.imageTitles, QStringList({"File:B.jpg"}));
        QCOMPARE(second.imageUrls, QStringList({"https://img.example/b.jpg"}));
        QVERIFY(second.imageContinuation.isEmpty());
    }

    void testImageMetadataRejectsInvalidEnvelope_data() {
        QTest::addColumn<QByteArray>("metadata");
        QTest::newRow("empty object") << QByteArray("{}");
        QTest::newRow("array") << QByteArray("[]");
        QTest::newRow("missing pages") << QByteArray(R"({"query":{}})");
        QTest::newRow("wrong pages type") << QByteArray(R"({"query":{"pages":[]}})");
        QTest::newRow("wrong page value") << QByteArray(R"({"query":{"pages":{"1":[]}}})");
        QTest::newRow("empty page object") << QByteArray(R"({"query":{"pages":{"1":{}}}})");
        QTest::newRow("wrong image info type") << QByteArray(R"({"query":{"pages":{"1":{"title":"File:A.jpg","imageinfo":[[]]}}}})");
        QTest::newRow("empty API error") << QByteArray(R"({"error":{}})");
    }

    void testImageMetadataRejectsInvalidEnvelope() {
        QFETCH(QByteArray, metadata);
        m_network.addFixture("GET", QUrl("https://en.wikipedia.org/w/api.php?action=query&format=json"
            "&prop=extracts|images&pageids=736&explaintext=1&imlimit=50"),
            R"({"query":{"pages":{"736":{"pageid":736,"title":"Gallery","images":[{"title":"File:A.jpg"}]}}}})");
        m_network.addFixture("GET", QUrl("https://en.wikipedia.org/w/api.php?action=query&format=json"
            "&prop=imageinfo&iiprop=url|extmetadata|mime&iiurlheight=330&iiurlwidth=330&titles=File:A.jpg"), metadata);
        WikipediaPageClient client;
        QSignalSpy pages(&client, &WikipediaPageClient::pageWithImagesReceived);
        QSignalSpy errors(&client, &WikipediaPageClient::errorOccurred);
        client.getPageWithImages(736);
        QTRY_COMPARE(errors.count(), 1);
        QVERIFY(!errors.first().first().toString().isEmpty());
        QCOMPARE(pages.count(), 0);
    }

    void testGalleryMetadataFailureCanRetry() {
        const QUrl pageUrl("https://en.wikipedia.org/w/api.php?action=query&format=json"
            "&prop=extracts|images&pageids=736&explaintext=1&imlimit=50");
        const QUrl metadataUrl("https://en.wikipedia.org/w/api.php?action=query&format=json"
            "&prop=imageinfo&iiprop=url|extmetadata|mime&iiurlheight=330&iiurlwidth=330&titles=File:A.jpg");
        m_network.addFixture("GET", pageUrl,
            R"({"query":{"pages":{"736":{"pageid":736,"title":"Gallery","images":[{"title":"File:A.jpg"}]}}}})");
        m_network.addFixture("GET", metadataUrl, "failure", "application/json", 500);
        WikipediaPageClient client;
        QSignalSpy pages(&client, &WikipediaPageClient::pageWithImagesReceived);
        QSignalSpy errors(&client, &WikipediaPageClient::errorOccurred);
        client.getPageWithImages(736);
        QTRY_COMPARE(errors.count(), 1);
        QCOMPARE(pages.count(), 0);

        m_network.addFixture("GET", metadataUrl,
            R"({"query":{"pages":{"1":{"title":"File:A.jpg","imageinfo":[{"url":"https:\/\/img.example/a.jpg"}]}}}})");
        client.getPageWithImages(736);
        QTRY_COMPARE(pages.count(), 1);
        QCOMPARE(errors.count(), 1);
        QCOMPARE(pages.first().first().value<page>().imageUrls, QStringList({"https://img.example/a.jpg"}));
    }

    void testObsoleteGalleryMetadataReplyIsIgnoredAndCanRecover() {
        const QUrl pageUrl("https://en.wikipedia.org/w/api.php?action=query&format=json"
            "&prop=extracts|images&pageids=736&explaintext=1&imlimit=50");
        const QUrl metadataUrl("https://en.wikipedia.org/w/api.php?action=query&format=json"
            "&prop=imageinfo&iiprop=url|extmetadata|mime&iiurlheight=330&iiurlwidth=330&titles=File:A.jpg");
        m_network.addFixture("GET", pageUrl,
            R"({"query":{"pages":{"736":{"pageid":736,"title":"Gallery","images":[{"title":"File:A.jpg"}]}}}})");
        m_network.addFixture("GET", metadataUrl,
            R"({"query":{"pages":{"1":{"title":"File:A.jpg","imageinfo":[{"url":"https:\/\/img.example/a.jpg"}]}}}})");
        WikipediaPageClient client;
        QSignalSpy pages(&client, &WikipediaPageClient::pageWithImagesReceived);
        QSignalSpy errors(&client, &WikipediaPageClient::errorOccurred);
        m_network.deferNextReply(true);
        client.getPageWithImages(736);
        m_network.deferNextReply(true);
        m_network.completeNextReply();
        QTRY_COMPARE(m_network.requestCount(), 2);
        QCOMPARE(m_network.pendingReplyCount(), 1);
        client.cancelPendingRequests();
        m_network.completeNextReply(); // Complete the one outstanding stale metadata response.
        QTRY_COMPARE(m_network.pendingReplyCount(), 0);
        QCOMPARE(pages.count(), 0);
        QCOMPARE(errors.count(), 0);

        client.getPageWithImages(736);
        QTRY_COMPARE(pages.count(), 1);
        QCOMPARE(errors.count(), 0);
        QCOMPARE(pages.first().first().value<page>().imageUrls, QStringList({"https://img.example/a.jpg"}));
    }

    void testSectionsRejectInvalidResponse_data() {
        QTest::addColumn<QByteArray>("response");
        QTest::newRow("malformed json") << QByteArray("not json");
        QTest::newRow("non-object json") << QByteArray("[]");
        QTest::newRow("api error") << QByteArray(R"({"error":{"code":"badvalue","info":"Invalid page"}})");
        QTest::newRow("empty api error with otherwise valid sections") << QByteArray(
            R"({"error":{},"parse":{"tocdata":{"sections":[]}}})");
        QTest::newRow("missing parse") << QByteArray("{}");
        QTest::newRow("missing tocdata") << QByteArray(R"({"parse":{}})");
        QTest::newRow("missing sections") << QByteArray(R"({"parse":{"tocdata":{}}})");
        QTest::newRow("wrong sections type") << QByteArray(R"({"parse":{"tocdata":{"sections":{}}}})");
    }

    void testSectionsRejectInvalidResponse() {
        QFETCH(QByteArray, response);
        m_network.addFixture("GET", QUrl("https://en.wikipedia.org/w/api.php?action=parse&format=json"
            "&prop=tocdata&page=Albert%20Einstein"), response);
        WikipediaPageClient client;
        QSignalSpy sections(&client, &WikipediaPageClient::sectionsReceived);
        QSignalSpy errors(&client, &WikipediaPageClient::errorOccurred);
        m_network.deferNextReply();

        client.getSections("Albert Einstein");
        m_network.completeNextReply();

        QTRY_COMPARE(errors.count(), 1);
        QVERIFY(!errors.first().first().toString().trimmed().isEmpty());
        QCOMPARE(sections.count(), 0);
        QCOMPARE(m_network.requestCount(), 1);
    }

    void testSectionsWithEmptyArraySucceeds() {
        m_network.addFixture("GET", QUrl("https://en.wikipedia.org/w/api.php?action=parse&format=json"
            "&prop=tocdata&page=Albert%20Einstein"),
            R"({"parse":{"tocdata":{"sections":[]}}})");
        WikipediaPageClient client;
        QSignalSpy sections(&client, &WikipediaPageClient::sectionsReceived);
        QSignalSpy errors(&client, &WikipediaPageClient::errorOccurred);

        client.getSections("Albert Einstein");

        QTRY_COMPARE(sections.count(), 1);
        QVERIFY(sections.first().first().value<QVector<section>>().isEmpty());
        QCOMPARE(errors.count(), 0);
        QCOMPARE(m_network.requestCount(), 1);
    }

    void testImageDescriptionEntities_data() {
        QTest::addColumn<QString>("html");
        QTest::addColumn<QString>("expected");
        QTest::newRow("decimal entities") << QString("<p>caf&#233; &#169;</p>") << QString("café ©");
        QTest::newRow("hexadecimal entity") << QString("<p>caf&#xE9;</p>") << QString("café");
        QTest::newRow("named entities") << QString("<p>&copy; &euro; &mdash;</p>") << QString("© € —");
        QTest::newRow("existing entities") << QString("<p>A &amp; B &lt; C &gt; D &quot;E&quot; &#39;F&#39;&nbsp;G</p>")
            << QString("A & B < C > D \"E\" 'F' G");
        QTest::newRow("apostrophe entities") << QString("<p>&apos; &#x27;</p>") << QString("' '");
        QTest::newRow("nested formatting") << QString("<p><b>Bold <i>and italic</i></b> text</p>")
            << QString("Bold and italic text");
        QTest::newRow("empty description") << QString() << QString();
        QTest::newRow("trim whitespace") << QString("  <p> Caption </p>  ") << QString("Caption");
        QTest::newRow("decode once") << QString("<p>&amp;lt;literal&amp;gt;</p>") << QString("&lt;literal&gt;");
    }

    void testImageDescriptionEntities() {
        QFETCH(QString, html);
        QFETCH(QString, expected);
        const QByteArray imageResponse = QJsonDocument(QJsonObject{
            {"query", QJsonObject{{"pages", QJsonObject{{"10", QJsonObject{
                {"title", "File:Caption.jpg"},
                {"imageinfo", QJsonArray{QJsonObject{
                    {"url", "https://example.org/caption.jpg"},
                    {"extmetadata", QJsonObject{{"ImageDescription", QJsonObject{{"value", html}}}}}
                }}}
            }}}}}}
        }).toJson(QJsonDocument::Compact);
        m_network.addFixture("GET", QUrl("https://en.wikipedia.org/w/api.php?action=query&format=json"
            "&prop=extracts|images&pageids=736&explaintext=1&imlimit=50"),
            R"({"query":{"pages":{"736":{"pageid":736,"title":"Captions","images":[{"title":"File:Caption.jpg"}]}}}})");
        m_network.addFixture("GET", QUrl("https://en.wikipedia.org/w/api.php?action=query&format=json"
            "&prop=imageinfo&iiprop=url|extmetadata|mime&iiurlheight=330&iiurlwidth=330&titles=File:Caption.jpg"),
            imageResponse);
        WikipediaPageClient client;
        QSignalSpy pages(&client, &WikipediaPageClient::pageWithImagesReceived);
        QSignalSpy errors(&client, &WikipediaPageClient::errorOccurred);
        client.getPageWithImages(736);
        QTRY_COMPARE(pages.count(), 1);
        const page result = pages.first().first().value<page>();
        QCOMPARE(result.imageDescriptions, QStringList{expected});
        QCOMPARE(errors.count(), 0);
        QCOMPARE(m_network.requestCount(), 2);
    }

    void testParsePageWithImagesNoImages() {
        QByteArray json = R"({"query":{"pages":{"736":{
            "title":"Albert Einstein",
            "extract":"A theoretical physicist.",
            "pageid":736
        }}}})";

        page p = WikipediaPageClient::parsePageWithImages(json, 736);
        QCOMPARE(p.title, QString("Albert Einstein"));
        QVERIFY(p.imageTitles.isEmpty());
        QVERIFY(p.imageUrls.isEmpty());
        QVERIFY(p.imageDescriptions.isEmpty());
    }

    void testParsePageWithImagesPageIdMismatch() {
        QByteArray json = R"({"query":{"pages":{"736":{
            "title":"Albert Einstein",
            "extract":"A theoretical physicist.",
            "pageid":736
        }}}})";

        page p = WikipediaPageClient::parsePageWithImages(json, 999);
        QVERIFY(p.title.isEmpty());
        QVERIFY(p.imageTitles.isEmpty());
        QVERIFY(p.imageUrls.isEmpty());
        QVERIFY(p.imageDescriptions.isEmpty());
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
