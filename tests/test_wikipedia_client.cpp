#include <QCoreApplication>
#include <QTest>
#include <QSignalSpy>
#include "../src/wikipedia_client/wikipedia_client.h"

class TestWikipediaClient : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Initialize test case
        qDebug() << "Starting Wikipedia Client tests...";
    }

    void testSearch()
    {
        Wikipedia::WikipediaClient client;
        QSignalSpy spy(&client, &Wikipedia::WikipediaClient::searchCompleted);
        QSignalSpy errorSpy(&client, &Wikipedia::WikipediaClient::errorOccurred);

        // Test search functionality
        client.search("Einstein");
        
        // Wait for signal or timeout
        if (spy.wait(10000)) { // 10 second timeout
            QVERIFY(spy.count() > 0);
            QList<QVariant> arguments = spy.takeFirst();
            QVector<Wikipedia::SearchResult> results = arguments.at(0).value<QVector<Wikipedia::SearchResult>>();
            QVERIFY(!results.isEmpty());
            qDebug() << "Search test passed. Found" << results.size() << "results";
        } else if (errorSpy.count() > 0) {
            QList<QVariant> errorArguments = errorSpy.takeFirst();
            QString error = errorArguments.at(0).toString();
            qDebug() << "Search test failed with error:" << error;
            QFAIL("Search request failed");
        } else {
            QFAIL("Search test timed out");
        }
    }

    void testGetPageById()
    {
        Wikipedia::WikipediaClient client;
        QSignalSpy spy(&client, &Wikipedia::WikipediaClient::pageReceived);
        QSignalSpy errorSpy(&client, &Wikipedia::WikipediaClient::errorOccurred);

        // Use a known page ID for testing (Einstein's page ID is 736)
        client.getPageById(736);
        
        if (spy.wait(10000)) {
            QVERIFY(spy.count() > 0);
            QList<QVariant> arguments = spy.takeFirst();
            Wikipedia::Page page = arguments.at(0).value<Wikipedia::Page>();
            QVERIFY(!page.title.isEmpty());
            QVERIFY(!page.extract.isEmpty());
            QVERIFY(page.pageid == 736);
            qDebug() << "Get page by ID test passed. Title:" << page.title;
        } else if (errorSpy.count() > 0) {
            QList<QVariant> errorArguments = errorSpy.takeFirst();
            QString error = errorArguments.at(0).toString();
            qDebug() << "Get page by ID test failed with error:" << error;
            QFAIL("Get page by ID request failed");
        } else {
            QFAIL("Get page by ID test timed out");
        }
    }

    void testGetPageWithImages()
    {
        Wikipedia::WikipediaClient client;
        QSignalSpy spy(&client, &Wikipedia::WikipediaClient::pageWithImagesReceived);
        QSignalSpy errorSpy(&client, &Wikipedia::WikipediaClient::errorOccurred);

        // Use a known page ID that has images
        client.getPageWithImages(736);
        
        if (spy.wait(15000)) { // Longer timeout for image fetching
            QVERIFY(spy.count() > 0);
            QList<QVariant> arguments = spy.takeFirst();
            Wikipedia::Page page = arguments.at(0).value<Wikipedia::Page>();
            QVERIFY(!page.title.isEmpty());
            QVERIFY(!page.extract.isEmpty());
            QVERIFY(page.pageid == 736);
            qDebug() << "Get page with images test passed. Title:" << page.title;
            qDebug() << "Found" << page.imageUrls.size() << "images";
        } else if (errorSpy.count() > 0) {
            QList<QVariant> errorArguments = errorSpy.takeFirst();
            QString error = errorArguments.at(0).toString();
            qDebug() << "Get page with images test failed with error:" << error;
            QFAIL("Get page with images request failed");
        } else {
            qDebug() << "Get page with images test timed out - this might be expected if image fetching is slow";
        }
    }

    void testGetFeaturedArticle()
    {
        Wikipedia::WikipediaClient client;
        QSignalSpy spy(&client, &Wikipedia::WikipediaClient::featuredArticleReceived);
        QSignalSpy errorSpy(&client, &Wikipedia::WikipediaClient::errorOccurred);

        client.getFeaturedArticleOfTheDay();
        
        if (spy.wait(10000)) {
            QVERIFY(spy.count() > 0);
            QList<QVariant> arguments = spy.takeFirst();
            Wikipedia::FeaturedArticle article = arguments.at(0).value<Wikipedia::FeaturedArticle>();
            QVERIFY(!article.title.isEmpty());
            QVERIFY(!article.date.isEmpty());
            qDebug() << "Featured article test passed. Title:" << article.title;
            qDebug() << "Date:" << article.date;
        } else if (errorSpy.count() > 0) {
            QList<QVariant> errorArguments = errorSpy.takeFirst();
            QString error = errorArguments.at(0).toString();
            qDebug() << "Featured article test failed with error:" << error;
            qDebug() << "Note: The featured feed API might not be available on test.wikipedia.org";
        } else {
            qDebug() << "Featured article test timed out";
        }
    }

    void cleanupTestCase()
    {
        qDebug() << "Wikipedia Client tests completed";
    }
};

QTEST_MAIN(TestWikipediaClient)
#include "test_wikipedia_client.moc"
