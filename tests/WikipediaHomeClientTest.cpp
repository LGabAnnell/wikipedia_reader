// tests/WikipediaHomeClientTest.cpp
#include <QtTest/QtTest>
#include <QByteArray>
#include <QFile>
#include <QRegularExpression>
#include "wikipedia_home_client.h"
#include "wikipedia_network_access_manager.h"
#include "support/FakeNetworkAccessManager.h"

namespace {
QStringList capturedWarnings;

void captureWarning(QtMsgType type, const QMessageLogContext &, const QString &message) {
    if (type == QtWarningMsg)
        capturedWarnings.append(message);
}

class ScopedWarningCapture {
  public:
    ScopedWarningCapture() {
        capturedWarnings.clear();
        m_previousHandler = qInstallMessageHandler(captureWarning);
    }

    ~ScopedWarningCapture() { qInstallMessageHandler(m_previousHandler); }

  private:
    QtMessageHandler m_previousHandler;
};

void verifyBoundedSelectedDiagnostic(const QByteArray &response, const QString &status) {
    QCOMPARE(capturedWarnings.size(), 1);
    const QString warning = capturedWarnings.first();
    QVERIFY(warning.size() < 256);
    QVERIFY(warning.contains(QString::number(response.size())));
    QVERIFY(warning.contains("byte", Qt::CaseInsensitive));
    QVERIFY(warning.contains("selected", Qt::CaseInsensitive));
    QVERIFY(warning.contains(status, Qt::CaseInsensitive));
    QVERIFY(!warning.contains(QString::fromUtf8(response)));
    QVERIFY(!warning.contains("other", Qt::CaseInsensitive));
    QVERIFY(!warning.contains("secret", Qt::CaseInsensitive));
    QVERIFY(!warning.contains(QRegularExpression("\\[[^]]*\\]")));
}
} // namespace

class WikipediaHomeClientTest : public QObject {
    Q_OBJECT

  private:
    NetworkFixtureController m_network;

    QUrl feedUrl(int daysBack = 0, const QString &language = "en") const {
        return QUrl(QString("https://api.wikimedia.org/feed/v1/wikipedia/%1/featured/%2")
                        .arg(language, QDate::currentDate().addDays(-daysBack).toString("yyyy/MM/dd")));
    }

    QByteArray newsFeed(const QString &title = "Today's news") const {
        return QJsonDocument(QJsonObject{{"news", QJsonArray{
            QJsonObject{{"story", title}, {"links", QJsonArray{
                QJsonObject{{"title", title}, {"pageid", 7}}
            }}}
        }}}).toJson();
    }

    QByteArray mostreadFeed(int count) const {
        QJsonArray articles;
        for (int i = 1; i <= count; ++i)
            articles.append(QJsonObject{{"title", QString("Article %1").arg(i)},
                                       {"extract", QString("Summary %1").arg(i)}, {"pageid", i}});
        return QJsonDocument(QJsonObject{{"mostread", QJsonObject{{"articles", articles}}}}).toJson();
    }

  private slots:
    void initTestCase() {
        connect(&m_network, &NetworkFixtureController::unexpectedRequest, this,
                [](const QString &request) { qFatal("%s", qPrintable(request)); });
        WikipediaNetwork::installNetworkAccessManagerFactory(&m_network);
    }

    void cleanupTestCase() { WikipediaNetwork::installNetworkAccessManagerFactory(nullptr); }
    void init() { m_network.clearRequests(); }
    void cleanup() { QCOMPARE(m_network.pendingReplyCount(), 0); }

    void testPartialFeedRecovery_data() {
        QTest::addColumn<int>("outcome");
        QTest::newRow("alternate succeeds") << 0;
        QTest::newRow("both partial") << 1;
        QTest::newRow("language changes during fallback") << 2;
        QTest::newRow("refresh cancels fallback") << 3;
    }

    void testPartialFeedRecovery() {
        QFETCH(int, outcome);
        const QUrl alternate(QString("https://en.wikipedia.org/api/rest_v1/feed/featured/%1")
                                 .arg(QDate::currentDate().toString("yyyy/MM/dd")));
        const QByteArray partial = R"({"onthisday":[]})";
        m_network.addFixture("GET", feedUrl(), partial);
        m_network.addFixture("GET", alternate, outcome == 1 ? partial : mostreadFeed(2));
        WikipediaHomeClient client;
        QSignalSpy received(&client, &WikipediaHomeClient::newsItemsReceived);
        QSignalSpy errors(&client, &WikipediaHomeClient::sectionError);
        m_network.deferNextReply();
        client.getNewsItems();
        m_network.deferNextReply(true);
        m_network.completeNextReply();
        QCOMPARE(m_network.requestCount(), 2);
        QCOMPARE(m_network.pendingReplyCount(), 1);
        QCOMPARE(received.count() + errors.count(), 0);
        if (outcome == 2)
            client.setLanguage("fr");
        else if (outcome == 3)
            client.cancelPendingRequests();
        m_network.completeNextReply();
        QCOMPARE(received.count(), outcome == 0 ? 1 : 0);
        QCOMPARE(errors.count(), outcome == 1 ? 1 : 0);
        QCOMPARE(m_network.requestCount(), 2);
        if (outcome == 0) {
            const auto items = received.first().first().value<QVector<news_item>>();
            QCOMPARE(items.size(), 2);
            QCOMPARE(items.first().title, QString("Article 1"));
        }
    }

    void testRecordedMostreadNeedsNoLookback_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QString>("firstTitle");
        QTest::addColumn<int>("firstPageId");
        QTest::newRow("English") << QString("en") << QString("Kim Jong Un") << 5962511;
        QTest::newRow("French") << QString("fr") << QString("David Lisnard") << 2023150;
    }

    void testRecordedMostreadNeedsNoLookback() {
        QFETCH(QString, language);
        QFETCH(QString, firstTitle);
        QFETCH(int, firstPageId);
        QFile fixture(QStringLiteral(FEED_FIXTURE_DIR "/%1-2026-10-21.json").arg(language));
        QVERIFY(fixture.open(QIODevice::ReadOnly));
        m_network.addFixture("GET", feedUrl(0, language), fixture.readAll());
        WikipediaHomeClient client;
        client.setLanguage(language);
        QSignalSpy items(&client, &WikipediaHomeClient::newsItemsReceived);
        QSignalSpy errors(&client, &WikipediaHomeClient::errorOccurred);
        client.getNewsItems();
        QTRY_COMPARE(items.count(), 1);
        const auto articles = items.first().first().value<QVector<news_item>>();
        QCOMPARE(articles.size(), 2);
        QCOMPARE(articles.first().title, firstTitle);
        QCOMPARE(articles.first().pageid, firstPageId);
        QVERIFY(!articles.first().description.isEmpty());
        QCOMPARE(QUrl(articles.first().url).host(), language + ".wikipedia.org");
        QVERIFY(articles.first().imageUrl.startsWith("https://thumb.wikimedia.org/"));
        QCOMPARE(errors.count(), 0);
        QCOMPARE(m_network.requestCount(), 1);
    }

    void testIncompleteTodayUsesPreviousMostread() {
        // Today's feed can have only four stories while yesterday has 43 articles.
        QJsonArray news;
        for (int i = 0; i < 4; ++i)
            news.append(QJsonDocument::fromJson(newsFeed()).object()["news"].toArray().first());
        m_network.addFixture("GET", feedUrl(), QJsonDocument(QJsonObject{{"news", news}}).toJson());
        m_network.addFixture("GET", feedUrl(1), mostreadFeed(43));
        WikipediaHomeClient client;
        QSignalSpy items(&client, &WikipediaHomeClient::newsItemsReceived);
        QSignalSpy errors(&client, &WikipediaHomeClient::errorOccurred);
        client.getNewsItems();
        QTRY_COMPARE(items.count(), 1);
        const auto articles = items.first().first().value<QVector<news_item>>();
        QCOMPARE(articles.size(), 43);
        QCOMPARE(articles.first().title, QString("Article 1"));
        QCOMPARE(articles.last().title, QString("Article 43"));
        QCOMPARE(articles.last().description, QString("Summary 43"));
        QCOMPARE(articles.last().pageid, 43);
        QCOMPARE(m_network.requestCount(), 2);
        QCOMPARE(errors.count(), 0);
    }

    void testLookbackSkipsUnavailableFeeds_data() {
        QTest::addColumn<int>("status");
        QTest::newRow("empty") << 200;
        QTest::newRow("not found") << 404;
        QTest::newRow("server failure") << 503;
    }

    void testLookbackSkipsUnavailableFeeds() {
        QFETCH(int, status);
        m_network.addFixture("GET", feedUrl(), newsFeed());
        m_network.addFixture("GET", feedUrl(1), "{}", "application/json", status);
        m_network.addFixture("GET", feedUrl(2), mostreadFeed(43));
        WikipediaHomeClient client;
        QSignalSpy items(&client, &WikipediaHomeClient::newsItemsReceived);
        client.getNewsItems();
        QTRY_COMPARE(items.count(), 1);
        QCOMPARE(items.first().first().value<QVector<news_item>>().size(), 43);
        QCOMPARE(m_network.requestCount(), 3);
    }

    void testLookbackIsBoundedAndRetainsTodaysNews_data() {
        QTest::addColumn<int>("status");
        QTest::newRow("no mostread") << 200;
        QTest::newRow("failed historical feeds") << 503;
    }

    void testLookbackIsBoundedAndRetainsTodaysNews() {
        QFETCH(int, status);
        m_network.addFixture("GET", feedUrl(), newsFeed());
        m_network.addFixture("GET", feedUrl(1), newsFeed("Yesterday's news"), "application/json", status);
        m_network.addFixture("GET", feedUrl(2), newsFeed("Older news"), "application/json", status);
        WikipediaHomeClient client;
        QSignalSpy items(&client, &WikipediaHomeClient::newsItemsReceived);
        QSignalSpy errors(&client, &WikipediaHomeClient::errorOccurred);
        client.getNewsItems();
        QTRY_COMPARE(items.count(), 1);
        const auto articles = items.first().first().value<QVector<news_item>>();
        QCOMPARE(articles.size(), 1);
        QCOMPARE(articles.first().title, QString("Today's news"));
        QCOMPARE(m_network.requestCount(), 3);
        QCOMPARE(errors.count(), 0);
    }

    void testCurrentFeedFailureEmitsError() {
        m_network.addFixture("GET", feedUrl(), "unavailable", "text/plain", 503);
        WikipediaHomeClient client;
        QSignalSpy items(&client, &WikipediaHomeClient::newsItemsReceived);
        QSignalSpy errors(&client, &WikipediaHomeClient::errorOccurred);
        QSignalSpy typedErrors(&client, &WikipediaHomeClient::sectionError);
        client.getNewsItems();
        QTRY_COMPARE(errors.count(), 1);
        QCOMPARE(items.count(), 0);
        QCOMPARE(typedErrors.count(), 1);
        QCOMPARE(typedErrors.first().at(0).value<WikipediaHomeClient::Section>(), WikipediaHomeClient::News);
        QCOMPARE(m_network.requestCount(), 1);
    }

    void testMalformedCurrentNewsFeedEmitsTypedFailure() {
        m_network.addFixture("GET", feedUrl(), "not json");
        WikipediaHomeClient client;
        QSignalSpy items(&client, &WikipediaHomeClient::newsItemsReceived);
        QSignalSpy errors(&client, &WikipediaHomeClient::sectionError);
        client.getNewsItems();
        QTRY_COMPARE(errors.count(), 1);
        QCOMPARE(items.count(), 0);
        QCOMPARE(errors.first().at(0).value<WikipediaHomeClient::Section>(), WikipediaHomeClient::News);
    }

    void testObsoleteLookbackDoesNotPublishOrContinue_data() {
        QTest::addColumn<bool>("changeLanguage");
        QTest::newRow("language change") << true;
        QTest::newRow("refresh canceled") << false;
    }

    void testObsoleteLookbackDoesNotPublishOrContinue() {
        QFETCH(bool, changeLanguage);
        m_network.addFixture("GET", feedUrl(), newsFeed());
        m_network.addFixture("GET", feedUrl(1), "{}");
        WikipediaHomeClient client;
        QSignalSpy items(&client, &WikipediaHomeClient::newsItemsReceived);
        QSignalSpy errors(&client, &WikipediaHomeClient::errorOccurred);
        m_network.deferNextReply();
        client.getNewsItems();
        m_network.deferNextReply(true);
        m_network.completeNextReply();
        QCOMPARE(m_network.requestCount(), 2);
        QCOMPARE(m_network.pendingReplyCount(), 1);
        if (changeLanguage)
            client.setLanguage("fr");
        else
            client.cancelPendingRequests();
        m_network.completeNextReply();
        QCoreApplication::processEvents();
        QCOMPARE(items.count(), 0);
        QCOMPARE(errors.count(), 0);
        QCOMPARE(m_network.requestCount(), 2);
    }

    void testParseNewsItems() {
        QByteArray json = R"({"mostread":{"articles":[
            {
                "title":"Article A",
                "extract":"Description A",
                "pageid":1,
                "content_urls":{"desktop":{"page":"https:\/\/en.wikipedia.org/wiki/Article_A"}}
            },
            {
                "title":"Article B",
                "extract":"Description B",
                "pageid":2,
                "content_urls":{"desktop":{"page":"https:\/\/en.wikipedia.org/wiki/Article_B"}}
            }
        ]}})";

        QVector<news_item> items = WikipediaHomeClient::parseNewsItems(json);
        QCOMPARE(items.size(), 2);
        QCOMPARE(items[0].title, QString("Article A"));
        QCOMPARE(items[0].description, QString("Description A"));
        QCOMPARE(items[0].pageid, 1);
        QCOMPARE(items[0].url, QString("https://en.wikipedia.org/wiki/Article_A"));
        QCOMPARE(items[1].title, QString("Article B"));
        QCOMPARE(items[1].description, QString("Description B"));
        QCOMPARE(items[1].pageid, 2);
        QCOMPARE(items[1].url, QString("https://en.wikipedia.org/wiki/Article_B"));
    }

    void testParseNewsItemsWithThumbnail() {
        QByteArray json = R"({"mostread":{"articles":[
            {
                "title":"Article C",
                "extract":"Description C",
                "pageid":3,
                "content_urls":{"desktop":{"page":"https:\/\/en.wikipedia.org/wiki/Article_C"}},
                "thumbnail":{"source":"https:\/\/upload.wikimedia.org/thumb.jpg"}
            }
        ]}})";

        QVector<news_item> items = WikipediaHomeClient::parseNewsItems(json);
        QCOMPARE(items.size(), 1);
        QCOMPARE(items[0].imageUrl, QString("https://upload.wikimedia.org/thumb.jpg"));
    }

    void testParseNewsStoriesWithoutMostread() {
        const QByteArray json = R"json({"news":[{
            "story":"A <b>new discovery</b> &amp; its implications.",
            "links":[{
                "title":"Article_A", "titles":{"normalized":"Article A"},
                "pageid":7,
                "content_urls":{"desktop":{"page":"https:\/\/fr.wikipedia.org/wiki/Article_A"}},
                "thumbnail":{"source":"https:\/\/upload.wikimedia.org/news.jpg"}
            }]
        }]})json";
        const auto items = WikipediaHomeClient::parseNewsItems(json);
        QCOMPARE(items.size(), 1);
        QCOMPARE(items[0].title, QString("Article A"));
        QCOMPARE(items[0].description, QString("A new discovery & its implications."));
        QCOMPARE(items[0].pageid, 7);
        QCOMPARE(items[0].url, QString("https://fr.wikipedia.org/wiki/Article_A"));
        QCOMPARE(items[0].imageUrl, QString("https://upload.wikimedia.org/news.jpg"));
    }

    void testMostreadTakesPrecedenceOverNews() {
        const QByteArray json = R"json({
            "news":[{"story":"News story", "links":[{"title":"News", "pageid":7}]}],
            "mostread":{"articles":[
                {"title":"Popular", "pageid":8, "extract":"Popular article summary"},
                {"title":"Another popular article", "pageid":9, "extract":"Another summary"}
            ]}
        })json";
        const auto items = WikipediaHomeClient::parseNewsItems(json);
        QCOMPARE(items.size(), 2);
        QCOMPARE(items[0].title, QString("Popular"));
        QCOMPARE(items[0].description, QString("Popular article summary"));
        QCOMPARE(items[0].pageid, 8);
        QVERIFY(items[0].imageUrl.isEmpty());
        QCOMPARE(items[1].title, QString("Another popular article"));
        QCOMPARE(items[1].description, QString("Another summary"));
        QCOMPARE(items[1].pageid, 9);
    }

    void testEmptyNewsDoesNotHideMostread() {
        const QByteArray json = R"json({
            "news":[], "mostread":{"articles":[{"title":"Popular", "pageid":8}]}
        })json";
        const auto items = WikipediaHomeClient::parseNewsItems(json);
        QCOMPARE(items.size(), 1);
        QCOMPARE(items[0].title, QString("Popular"));
        QCOMPARE(items[0].pageid, 8);
    }

    void testNewsFallbackWithoutMostreadArticles_data() {
        QTest::addColumn<QByteArray>("mostread");
        QTest::newRow("null") << QByteArray("null");
        QTest::newRow("missing articles") << QByteArray("{}");
        QTest::newRow("empty articles") << QByteArray(R"({"articles":[]})");
        QTest::newRow("invalid articles") << QByteArray(R"({"articles":{}})");
    }

    void testNewsFallbackWithoutMostreadArticles() {
        QFETCH(QByteArray, mostread);
        const QByteArray json = R"({"mostread":)" + mostread + R"(,
            "news":[{"story":"A <b>news</b> story", "links":[{"title":"News", "pageid":7}]}]
        })";
        const auto items = WikipediaHomeClient::parseNewsItems(json);
        QCOMPARE(items.size(), 1);
        QCOMPARE(items[0].title, QString("News"));
        QCOMPARE(items[0].description, QString("A news story"));
        QCOMPARE(items[0].pageid, 7);
    }

    void testNewsWithoutLinkedArticles() {
        const QByteArray json = R"json({
            "news":[{"story":"No links"}, {"story":"Empty links", "links":[]}]
        })json";
        QVERIFY(WikipediaHomeClient::parseNewsItems(json).isEmpty());
    }

    void testParseNewsItemsMissingMostread() {
        QByteArray json = R"({"other":"data"})";
        QVector<news_item> items = WikipediaHomeClient::parseNewsItems(json);
        QVERIFY(items.isEmpty());
    }

    void testParseNewsItemsMalformedJson() {
        QByteArray json = "not json";
        QVector<news_item> items = WikipediaHomeClient::parseNewsItems(json);
        QVERIFY(items.isEmpty());
    }

    void testOnThisDayUsesRequestedDate_data() {
        QTest::addColumn<int>("month");
        QTest::addColumn<int>("day");
        QTest::addColumn<QString>("language");
        QTest::addColumn<QString>("url");
        QTest::newRow("padded date") << 1 << 2 << QString("en")
            << QString("https://api.wikimedia.org/feed/v1/wikipedia/en/onthisday/all/01/02");
        QTest::newRow("double digit date and selected edition") << 12 << 31 << QString("fr")
            << QString("https://api.wikimedia.org/feed/v1/wikipedia/fr/onthisday/all/12/31");
    }

    void testOnThisDayUsesRequestedDate() {
        QFETCH(int, month);
        QFETCH(int, day);
        QFETCH(QString, language);
        QFETCH(QString, url);
        m_network.addFixture("GET", QUrl(url), R"({"selected":[]})");
        WikipediaHomeClient client;
        client.setLanguage(language);
        QSignalSpy events(&client, &WikipediaHomeClient::onThisDayEventsReceived);
        QSignalSpy errors(&client, &WikipediaHomeClient::errorOccurred);

        client.getOnThisDayEvents(month, day);

        QTRY_COMPARE(events.count(), 1);
        QVERIFY(events.first().first().value<QVector<on_this_day_event>>().isEmpty());
        QCOMPARE(errors.count(), 0);
        QCOMPARE(m_network.requestCount(), 1);
    }

    void testParseOnThisDayEvents() {
        QByteArray json = R"({"selected":[
            {
                "year":1969,
                "text":"Apollo 11 landed on the Moon.",
                "pages":[
                    {
                        "pageid":736,
                        "content_urls":{"desktop":{"page":"https:\/\/en.wikipedia.org/wiki/Apollo_11"}}
                    }
                ]
            },
            {
                "year":1945,
                "text":"End of World War II.",
                "pages":[
                    {
                        "pageid":999,
                        "content_urls":{"desktop":{"page":"https:\/\/en.wikipedia.org/wiki/WWII"}}
                    }
                ]
            }
        ]})";

        QVector<on_this_day_event> events = WikipediaHomeClient::parseOnThisDayEvents(json);
        QCOMPARE(events.size(), 2);
        QCOMPARE(events[0].year, 1969);
        QCOMPARE(events[0].event, QString("Apollo 11 landed on the Moon."));
        QCOMPARE(events[0].pageid, 736);
        QCOMPARE(events[0].url, QString("https://en.wikipedia.org/wiki/Apollo_11"));
        QCOMPARE(events[1].year, 1945);
        QCOMPARE(events[1].event, QString("End of World War II."));
        QCOMPARE(events[1].pageid, 999);
        QCOMPARE(events[1].url, QString("https://en.wikipedia.org/wiki/WWII"));
    }

    void testParseOnThisDayEventsEmptyPages() {
        QByteArray json = R"({"selected":[
            {"year":2000,"text":"Some event","pages":[]}
        ]})";

        QVector<on_this_day_event> events = WikipediaHomeClient::parseOnThisDayEvents(json);
        QCOMPARE(events.size(), 1);
        QCOMPARE(events[0].year, 2000);
        QCOMPARE(events[0].pageid, 0);
        QVERIFY(events[0].url.isEmpty());
    }

    void testParseOnThisDayEventsMissingSelected() {
        QByteArray json = R"({"other":"data"})";
        QVector<on_this_day_event> events;
        {
            ScopedWarningCapture capture;
            events = WikipediaHomeClient::parseOnThisDayEvents(json);
        }
        QVERIFY(events.isEmpty());
        verifyBoundedSelectedDiagnostic(json, "missing");
    }

    void testParseOnThisDayEventsWrongTypeSelected_data() {
        QTest::addColumn<QByteArray>("selectedJson");
        QTest::addColumn<QString>("expectedType");
        QTest::newRow("null") << QByteArray("null") << QString("null");
        QTest::newRow("boolean") << QByteArray("true") << QString("boolean");
        QTest::newRow("number") << QByteArray("42") << QString("number");
        QTest::newRow("string") << QByteArray("\"secret selected payload\"") << QString("string");
        QTest::newRow("object") << QByteArray(R"({"secret":"value"})") << QString("object");
    }

    void testParseOnThisDayEventsWrongTypeSelected() {
        QFETCH(QByteArray, selectedJson);
        QFETCH(QString, expectedType);
        const QByteArray json = QByteArray("{\"selected\":") + selectedJson + ",\"other\":\"secret key\"}";
        QVector<on_this_day_event> events;
        {
            ScopedWarningCapture capture;
            events = WikipediaHomeClient::parseOnThisDayEvents(json);
        }
        QVERIFY(events.isEmpty());
        verifyBoundedSelectedDiagnostic(json, expectedType);
    }

    void testParseOnThisDayEventsMalformedJson() {
        QByteArray json = "not json";
        QVector<on_this_day_event> events;
        {
            ScopedWarningCapture capture;
            events = WikipediaHomeClient::parseOnThisDayEvents(json);
        }
        QVERIFY(events.isEmpty());
        verifyBoundedSelectedDiagnostic(json, "missing");
    }

    void testParseOnThisDayEventsLargeUnexpectedResponse() {
        const QByteArray payload(128 * 1024, 'x');
        const QByteArray json = QByteArray("{\"selected\":\"secret\",\"payload\":\"") + payload + "\"}";
        QVector<on_this_day_event> events;
        {
            ScopedWarningCapture capture;
            events = WikipediaHomeClient::parseOnThisDayEvents(json);
        }
        QVERIFY(events.isEmpty());
        verifyBoundedSelectedDiagnostic(json, "string");
    }

    void testParseArticleContent() {
        QByteArray json = R"({
            "extract":"An article about something.",
            "content_urls":{"desktop":{"page":"https:\/\/en.wikipedia.org/wiki/Something"}},
            "pageid":42
        })";

        did_you_know_item dyk = WikipediaHomeClient::parseArticleContent(json);
        QCOMPARE(dyk.text, QString("An article about something."));
        QCOMPARE(dyk.url, QString("https://en.wikipedia.org/wiki/Something"));
        QCOMPARE(dyk.pageid, 42);
    }

    void testParseArticleContentMissingFields() {
        QByteArray json = R"({})";

        did_you_know_item dyk = WikipediaHomeClient::parseArticleContent(json);
        QVERIFY(dyk.text.isEmpty());
        QVERIFY(dyk.url.isEmpty());
        QCOMPARE(dyk.pageid, 0);
    }

    void testParseArticleContentMalformedJson() {
        QByteArray json = "not json";

        did_you_know_item dyk = WikipediaHomeClient::parseArticleContent(json);
        QVERIFY(dyk.text.isEmpty());
        QVERIFY(dyk.url.isEmpty());
        QCOMPARE(dyk.pageid, 0);
    }
};

QTEST_GUILESS_MAIN(WikipediaHomeClientTest)
#include "WikipediaHomeClientTest.moc"
