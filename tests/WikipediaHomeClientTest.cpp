// tests/WikipediaHomeClientTest.cpp
#include <QtTest/QtTest>
#include <QByteArray>
#include "wikipedia_home_client.h"

class WikipediaHomeClientTest : public QObject {
    Q_OBJECT

  private slots:
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
        QCOMPARE(items[0].url, QString("https:\/\/en.wikipedia.org/wiki/Article_A"));
        QCOMPARE(items[1].title, QString("Article B"));
        QCOMPARE(items[1].description, QString("Description B"));
        QCOMPARE(items[1].pageid, 2);
        QCOMPARE(items[1].url, QString("https:\/\/en.wikipedia.org/wiki/Article_B"));
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
        QCOMPARE(items[0].imageUrl, QString("https:\/\/upload.wikimedia.org/thumb.jpg"));
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
        QCOMPARE(events[0].url, QString("https:\/\/en.wikipedia.org/wiki/Apollo_11"));
        QCOMPARE(events[1].year, 1945);
        QCOMPARE(events[1].event, QString("End of World War II."));
        QCOMPARE(events[1].pageid, 999);
        QCOMPARE(events[1].url, QString("https:\/\/en.wikipedia.org/wiki/WWII"));
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
        QVector<on_this_day_event> events = WikipediaHomeClient::parseOnThisDayEvents(json);
        QVERIFY(events.isEmpty());
    }

    void testParseOnThisDayEventsMalformedJson() {
        QByteArray json = "not json";
        QVector<on_this_day_event> events = WikipediaHomeClient::parseOnThisDayEvents(json);
        QVERIFY(events.isEmpty());
    }

    void testParseArticleContent() {
        QByteArray json = R"({
            "extract":"An article about something.",
            "content_urls":{"desktop":{"page":"https:\/\/en.wikipedia.org/wiki/Something"}},
            "pageid":42
        })";

        did_you_know_item dyk = WikipediaHomeClient::parseArticleContent(json);
        QCOMPARE(dyk.text, QString("An article about something."));
        QCOMPARE(dyk.url, QString("https:\/\/en.wikipedia.org/wiki/Something"));
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

QTEST_MAIN(WikipediaHomeClientTest)
#include "WikipediaHomeClientTest.moc"
