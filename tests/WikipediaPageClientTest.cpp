// tests/WikipediaPageClientTest.cpp
#include <QtTest/QtTest>
#include <QByteArray>
#include "wikipedia_page_client.h"

class WikipediaPageClientTest : public QObject {
    Q_OBJECT

  private slots:
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

    void testParseSectionsEmpty() {
        QByteArray json = R"({"parse":{"tocdata":{"sections":[]}}})";
        QVector<section> sections = WikipediaPageClient::parseSections(json);
        QVERIFY(sections.isEmpty());
    }

    void testParseSectionsMissingParse() {
        QByteArray json = R"({})";
        QVector<section> sections = WikipediaPageClient::parseSections(json);
        QVERIFY(sections.isEmpty());
    }

    void testParseSectionsMalformedJson() {
        QByteArray json = "not json";
        QVector<section> sections = WikipediaPageClient::parseSections(json);
        QVERIFY(sections.isEmpty());
    }
};

QTEST_MAIN(WikipediaPageClientTest)
#include "WikipediaPageClientTest.moc"
