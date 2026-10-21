// tests/WikipediaFeaturedClientTest.cpp
#include "wikipedia_featured_client.h"
#include "wikipedia_network_access_manager.h"
#include "support/FakeNetworkAccessManager.h"
#include <QByteArray>
#include <QDate>
#include <QFile>
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
        // Qt moc can silently omit later slots after an unescaped URL in a
        // multiline raw string. Fail instead of reporting a partial suite green.
        QVERIFY2(metaObject()->indexOfSlot("testApiErrorObjectEmitsError()") >= 0,
                 "Qt moc did not register the final featured test slot");
        connect(&m_network, &NetworkFixtureController::unexpectedRequest, this,
                [](const QString &request) { qFatal("%s", qPrintable(request)); });
        WikipediaNetwork::installNetworkAccessManagerFactory(&m_network);
    }
    void cleanupTestCase() { WikipediaNetwork::installNetworkAccessManagerFactory(nullptr); }
    void init() { m_network.clearRequests(); }

    void testRecordedEditionFeed_data() {
        QTest::addColumn<QString>("language");
        QTest::newRow("English") << QString("en");
        QTest::newRow("French") << QString("fr");
    }

    void testRecordedEditionFeed() {
        QFETCH(QString, language);
        const QUrl url(QString("https://api.wikimedia.org/feed/v1/wikipedia/%1/featured/%2")
                           .arg(language, QDate::currentDate().toString("yyyy/MM/dd")));
        QFile fixture(QStringLiteral(FEED_FIXTURE_DIR "/%1-2026-10-21.json").arg(language));
        QVERIFY(fixture.open(QIODevice::ReadOnly));
        m_network.addFixture("GET", url, fixture.readAll());
        WikipediaFeaturedClient client;
        client.setLanguage(language);
        QSignalSpy received(&client, &WikipediaFeaturedClient::featuredArticleReceived);
        QSignalSpy errors(&client, &WikipediaFeaturedClient::errorOccurred);
        QSignalSpy unavailable(&client, &WikipediaFeaturedClient::featuredArticleUnavailable);
        client.getFeaturedArticleOfTheDay();
        if (language == "en") {
            QTRY_COMPARE(received.count(), 1);
            QCOMPARE(received.first().at(0).toString(), QString("Hongwu_Emperor"));
            QVERIFY(received.first().at(1).toString().contains("Ming dynasty"));
            QCOMPARE(received.first().at(2).toInt(), 163029);
            QVERIFY(received.first().at(3).toString().startsWith("https://thumb.wikimedia.org/"));
            QCOMPARE(unavailable.count(), 0);
        } else {
            QTRY_COMPARE(unavailable.count(), 1);
            QCOMPARE(received.count(), 0);
        }
        QCOMPARE(errors.count(), 0);
        QCOMPARE(m_network.requestCount(), 1);
    }

    void testParseFeaturedArticle() {
        // Escape JSON slashes so Qt moc does not interpret the URL as a comment.
        QByteArray json = R"json({"tfa":{
            "extract":"Albert Einstein was a theoretical physicist.",
            "title":"Albert Einstein",
            "pageid":736,
            "thumbnail":{"source":"https:\/\/upload.wikimedia.org/featured.jpg"}
        }})json";

        QString title, extract, thumbnail;
        int pageid;
        WikipediaFeaturedClient::parseFeaturedArticle(json, title, extract, pageid, thumbnail);

        QCOMPARE(title, QString("Albert Einstein"));
        QCOMPARE(extract, QString("Albert Einstein was a theoretical physicist."));
        QCOMPARE(pageid, 736);
        QCOMPARE(thumbnail, QString("https://upload.wikimedia.org/featured.jpg"));
    }

    void testParseFeaturedArticleMissingTfa() {
        QByteArray json = R"json({"other":"data"})json";

        QString title, extract, thumbnail;
        int pageid;
        WikipediaFeaturedClient::parseFeaturedArticle(json, title, extract, pageid, thumbnail);

        QVERIFY(title.isEmpty());
        QVERIFY(extract.isEmpty());
        QCOMPARE(pageid, 0);
    }

    void testParseFeaturedArticlePartialFields() {
        QByteArray json = R"json({"tfa":{
            "title":"Some Article"
        }})json";

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
        m_network.addFixture("GET", featuredUrl(), R"json({"tfa":null})json");
        WikipediaFeaturedClient client;
        QSignalSpy unavailable(&client, &WikipediaFeaturedClient::featuredArticleUnavailable);
        QSignalSpy errors(&client, &WikipediaFeaturedClient::errorOccurred);
        client.getFeaturedArticleOfTheDay();
        QTRY_COMPARE(unavailable.count(), 1);
        QCOMPARE(errors.count(), 0);
    }

    void testInvalidFeaturedArticleIsError() {
        m_network.addFixture("GET", featuredUrl(), R"json({"tfa":{"title":"No ID","pageid":0}})json");
        WikipediaFeaturedClient client;
        QSignalSpy received(&client, &WikipediaFeaturedClient::featuredArticleReceived);
        QSignalSpy errors(&client, &WikipediaFeaturedClient::errorOccurred);
        client.getFeaturedArticleOfTheDay();
        QTRY_COMPARE(errors.count(), 1);
        QCOMPARE(received.count(), 0);
    }

    void testApiErrorObjectEmitsError() {
        m_network.addFixture("GET", featuredUrl(), R"json({"error":{"code":"unavailable"}})json");
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
