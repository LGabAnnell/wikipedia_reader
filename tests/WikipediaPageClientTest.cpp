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
