// tests/WikipediaSearchClientTest.cpp
#include <QtTest/QtTest>
#include <QByteArray>
#include "wikipedia_search_client.h"

class WikipediaSearchClientTest : public QObject {
    Q_OBJECT

private slots:
    void testParseSearchResults_data() {
        QTest::addColumn<QByteArray>("json");
        QTest::addColumn<int>("expectedCount");

        QTest::newRow("empty results")
            << QByteArray(R"({"query":{"search":[]}})") << 0;

        QTest::newRow("one result")
            << QByteArray(R"({"query":{"search":[{"title":"Einstein","snippet":"...","pageid":736}]}})")
            << 1;

        QTest::newRow("three results")
            << QByteArray(R"({"query":{"search":[
                {"title":"A","snippet":"sa","pageid":1},
                {"title":"B","snippet":"sb","pageid":2},
                {"title":"C","snippet":"sc","pageid":3}
            ]}})")
            << 3;

        QTest::newRow("missing query key")
            << QByteArray("{}") << 0;

        QTest::newRow("malformed JSON")
            << QByteArray("not json") << 0;
    }

    void testParseSearchResults() {
        QFETCH(QByteArray, json);
        QFETCH(int, expectedCount);

        QVector<search_result> results = WikipediaSearchClient::parseSearchResults(json);
        QCOMPARE(results.size(), expectedCount);
    }

    void testParseOneResultFields() {
        QByteArray json = R"({"query":{"search":[
            {"title":"Albert Einstein","snippet":"physicist","pageid":736}
        ]}})";

        QVector<search_result> results = WikipediaSearchClient::parseSearchResults(json);
        QCOMPARE(results.size(), 1);
        QCOMPARE(results[0].title, QString("Albert Einstein"));
        QCOMPARE(results[0].snippet, QString("physicist"));
        QCOMPARE(results[0].pageid, 736);
    }
};

QTEST_MAIN(WikipediaSearchClientTest)
#include "WikipediaSearchClientTest.moc"
