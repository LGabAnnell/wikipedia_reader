// tests/WikipediaFeaturedClientTest.cpp
#include <QtTest/QtTest>
#include <QByteArray>
#include "wikipedia_featured_client.h"

class WikipediaFeaturedClientTest : public QObject {
    Q_OBJECT

  private slots:
    void testParseFeaturedArticle() {
        QByteArray json = R"({"tfa":{
            "extract":"Albert Einstein was a theoretical physicist.",
            "title":"Albert Einstein",
            "pageid":736
        }})";

        QString title, extract;
        int pageid;
        WikipediaFeaturedClient::parseFeaturedArticle(json, title, extract, pageid);

        QCOMPARE(title, QString("Albert Einstein"));
        QCOMPARE(extract, QString("Albert Einstein was a theoretical physicist."));
        QCOMPARE(pageid, 736);
    }

    void testParseFeaturedArticleMissingTfa() {
        QByteArray json = R"({"other":"data"})";

        QString title, extract;
        int pageid;
        WikipediaFeaturedClient::parseFeaturedArticle(json, title, extract, pageid);

        QVERIFY(title.isEmpty());
        QVERIFY(extract.isEmpty());
        QCOMPARE(pageid, 0);
    }

    void testParseFeaturedArticlePartialFields() {
        QByteArray json = R"({"tfa":{
            "title":"Some Article"
        }})";

        QString title, extract;
        int pageid;
        WikipediaFeaturedClient::parseFeaturedArticle(json, title, extract, pageid);

        QCOMPARE(title, QString("Some Article"));
        QVERIFY(extract.isEmpty());
        QCOMPARE(pageid, 0);
    }

    void testParseFeaturedArticleMalformedJson() {
        QByteArray json = "not json";

        QString title, extract;
        int pageid;
        WikipediaFeaturedClient::parseFeaturedArticle(json, title, extract, pageid);

        QVERIFY(title.isEmpty());
        QVERIFY(extract.isEmpty());
        QCOMPARE(pageid, 0);
    }
};

QTEST_MAIN(WikipediaFeaturedClientTest)
#include "WikipediaFeaturedClientTest.moc"
