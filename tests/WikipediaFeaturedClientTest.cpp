// tests/WikipediaFeaturedClientTest.cpp
#include "wikipedia_featured_client.h"
#include "wikipedia_network_access_manager.h"
#include "support/FakeNetworkAccessManager.h"
#include <QByteArray>
#include <QDate>
#include <QtTest/QtTest>

class WikipediaFeaturedClientTest : public QObject {
    Q_OBJECT

  private:
    NetworkFixtureController m_network;

    QUrl featuredUrl() const {
        return QUrl(QString("https://api.wikimedia.org/feed/v1/wikipedia/en/featured/%1")
                        .arg(QDate::currentDate().toString("yyyy/MM/dd")));
    }

  private slots:
    void initTestCase() {
        connect(&m_network, &NetworkFixtureController::unexpectedRequest, this,
                [](const QString &request) { qFatal("%s", qPrintable(request)); });
        WikipediaNetwork::installNetworkAccessManagerFactory(&m_network);
    }
    void cleanupTestCase() { WikipediaNetwork::installNetworkAccessManagerFactory(nullptr); }
    void init() { m_network.clearRequests(); }

    void testParseFeaturedArticle() {
        QByteArray json = R"({"tfa":{
            "extract":"Albert Einstein was a theoretical physicist.",
            "title":"Albert Einstein",
            "pageid":736,
            "thumbnail":{"source":"https://upload.wikimedia.org/featured.jpg"}
        }})";

        QString title, extract, thumbnail;
        int pageid;
        WikipediaFeaturedClient::parseFeaturedArticle(json, title, extract, pageid, thumbnail);

        QCOMPARE(title, QString("Albert Einstein"));
        QCOMPARE(extract, QString("Albert Einstein was a theoretical physicist."));
        QCOMPARE(pageid, 736);
        QCOMPARE(thumbnail, QString("https://upload.wikimedia.org/featured.jpg"));
    }

    void testParseFeaturedArticleMissingTfa() {
        QByteArray json = R"({"other":"data"})";

        QString title, extract, thumbnail;
        int pageid;
        WikipediaFeaturedClient::parseFeaturedArticle(json, title, extract, pageid, thumbnail);

        QVERIFY(title.isEmpty());
        QVERIFY(extract.isEmpty());
        QCOMPARE(pageid, 0);
    }

    void testParseFeaturedArticlePartialFields() {
        QByteArray json = R"({"tfa":{
            "title":"Some Article"
        }})";

        QString title, extract, thumbnail;
        int pageid;
        WikipediaFeaturedClient::parseFeaturedArticle(json, title, extract, pageid, thumbnail);

        QCOMPARE(title, QString("Some Article"));
        QVERIFY(extract.isEmpty());
        QCOMPARE(pageid, 0);
    }

    void testParseFeaturedArticleMalformedJson() {
        QByteArray json = "not json";

        QString title, extract, thumbnail;
        int pageid;
        WikipediaFeaturedClient::parseFeaturedArticle(json, title, extract, pageid, thumbnail);

        QVERIFY(title.isEmpty());
        QVERIFY(extract.isEmpty());
        QCOMPARE(pageid, 0);
    }

    void testMalformedResponseEmitsError() {
        m_network.addFixture("GET", featuredUrl(), "not json");
        WikipediaFeaturedClient client;
        QSignalSpy received(&client, &WikipediaFeaturedClient::featuredArticleReceived);
        QSignalSpy unavailable(&client, &WikipediaFeaturedClient::featuredArticleUnavailable);
        QSignalSpy errors(&client, &WikipediaFeaturedClient::errorOccurred);
        client.getFeaturedArticleOfTheDay();
        QTRY_COMPARE(errors.count(), 1);
        QCOMPARE(received.count(), 0);
        QCOMPARE(unavailable.count(), 0);
    }

    void testMissingTfaIsUnavailable() {
        m_network.addFixture("GET", featuredUrl(), R"({"tfa":null})");
        WikipediaFeaturedClient client;
        QSignalSpy unavailable(&client, &WikipediaFeaturedClient::featuredArticleUnavailable);
        QSignalSpy errors(&client, &WikipediaFeaturedClient::errorOccurred);
        client.getFeaturedArticleOfTheDay();
        QTRY_COMPARE(unavailable.count(), 1);
        QCOMPARE(errors.count(), 0);
    }

    void testInvalidFeaturedArticleIsError() {
        m_network.addFixture("GET", featuredUrl(), R"({"tfa":{"title":"No ID","pageid":0}})");
        WikipediaFeaturedClient client;
        QSignalSpy received(&client, &WikipediaFeaturedClient::featuredArticleReceived);
        QSignalSpy errors(&client, &WikipediaFeaturedClient::errorOccurred);
        client.getFeaturedArticleOfTheDay();
        QTRY_COMPARE(errors.count(), 1);
        QCOMPARE(received.count(), 0);
    }

    void testApiErrorObjectEmitsError() {
        m_network.addFixture("GET", featuredUrl(), R"({"error":{"code":"unavailable"}})");
        WikipediaFeaturedClient client;
        QSignalSpy received(&client, &WikipediaFeaturedClient::featuredArticleReceived);
        QSignalSpy errors(&client, &WikipediaFeaturedClient::errorOccurred);
        client.getFeaturedArticleOfTheDay();
        QTRY_COMPARE(errors.count(), 1);
        QCOMPARE(received.count(), 0);
    }
};

QTEST_GUILESS_MAIN(WikipediaFeaturedClientTest)
#include "WikipediaFeaturedClientTest.moc"
