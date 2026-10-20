#include "ImageSelectionState.h"
#include "support/FakeNetworkAccessManager.h"
#include "wikipedia_page_client.h"
#include "wikipedia_network_access_manager.h"
#include <QSignalSpy>
#include <QUrlQuery>
#include <QtTest/QTest>

class ImageSelectionStateTest : public QObject {
    Q_OBJECT

  private slots:
    void cleanup() {
        WikipediaNetwork::installNetworkAccessManagerFactory(nullptr);
    }

    void atomicSelection() {
        SettingsState settings;
        ImageSelectionState selection(settings);
        QVERIFY(selection.currentImageUrl().isEmpty());
        QVERIFY(selection.currentImageDescription().isEmpty());
        QSignalSpy urlSpy(&selection, &ImageSelectionState::currentImageUrlChanged);
        QSignalSpy captionSpy(&selection, &ImageSelectionState::currentImageDescriptionChanged);
        connect(&selection, &ImageSelectionState::currentImageUrlChanged, this, [&]() {
            QCOMPARE(selection.currentImageDescription(), QString("caption"));
        });
        connect(&selection, &ImageSelectionState::currentImageDescriptionChanged, this, [&]() {
            QCOMPARE(selection.currentImageUrl(), QString("image.png"));
        });
        selection.selectImage("image.png", "caption");
        QCOMPARE(urlSpy.count(), 1);
        QCOMPARE(captionSpy.count(), 1);
        selection.selectImage("image.png", "caption");
        QCOMPARE(urlSpy.count(), 1);
        QCOMPARE(captionSpy.count(), 1);
    }

    void languageTransitionClearsSelection() {
        SettingsState settings;
        ImageSelectionState selection(settings);
        selection.selectImage("old.png", "Old edition caption");
        settings.setLanguage("fr");
        QVERIFY(selection.currentImageUrl().isEmpty());
        QVERIFY(selection.currentImageDescription().isEmpty());
        QCOMPARE(selection.currentGalleryIndex(), -1);
        QCOMPARE(selection.galleryImageCount(), 0);
    }

    void galleryNavigationStopsAtEndsAndStandaloneClearsContext() {
        SettingsState settings;
        ImageSelectionState selection(settings);
        const QStringList urls{"one.png", "two.png", "three.png"};
        const QStringList captions{"one", "two", "three"};
        selection.selectGalleryImage(urls, captions, {}, 1);
        QCOMPARE(selection.currentImageUrl(), QString("two.png"));
        QCOMPARE(selection.currentGalleryIndex(), 1);
        QCOMPARE(selection.galleryImageCount(), 3);

        selection.selectNextImage();
        QCOMPARE(selection.currentImageUrl(), QString("three.png"));
        selection.selectNextImage();
        QCOMPARE(selection.currentImageUrl(), QString("three.png"));
        selection.selectPreviousImage();
        QCOMPARE(selection.currentImageUrl(), QString("two.png"));
        selection.selectPreviousImage();
        selection.selectPreviousImage();
        QCOMPARE(selection.currentImageUrl(), QString("one.png"));
        QCOMPARE(selection.currentGalleryIndex(), 0);

        selection.selectImage("inline.png", "inline caption");
        QCOMPARE(selection.currentGalleryIndex(), -1);
        QCOMPARE(selection.galleryImageCount(), 0);
        selection.selectNextImage();
        QCOMPARE(selection.currentImageUrl(), QString("inline.png"));
    }

    void emptyAndSingleImageGalleryNavigationIsSafe() {
        SettingsState settings;
        ImageSelectionState selection(settings);
        selection.selectGalleryImage({}, {}, {}, 0);
        QCOMPARE(selection.currentGalleryIndex(), -1);
        selection.selectGalleryImage({"single.png"}, {"caption"}, {}, 0);
        selection.selectNextImage();
        selection.selectPreviousImage();
        QCOMPARE(selection.currentImageUrl(), QString("single.png"));
        QCOMPARE(selection.currentGalleryIndex(), 0);
    }

    void waitsForDetailsAndSkipsIdenticalSelection() {
        NetworkFixtureController network;
        connect(&network, &NetworkFixtureController::unexpectedRequest, this,
                [](const QString &request) { qFatal("%s", qPrintable(request)); });
        WikipediaNetwork::installNetworkAccessManagerFactory(&network);
        const QString title = "File:One.jpg";
        QUrl requestUrl("https://en.wikipedia.org/w/api.php");
        QUrlQuery query;
        query.addQueryItem("action", "query");
        query.addQueryItem("format", "json");
        query.addQueryItem("prop", "imageinfo");
        query.addQueryItem("iiprop", "url|extmetadata|mime");
        query.addQueryItem("iiurlheight", "2048");
        query.addQueryItem("iiextmetadatafilter", "ImageDescription");
        query.addQueryItem("iiurlwidth", "2048");
        query.addQueryItem("titles", title);
        requestUrl.setQuery(query);
        network.addFixture("GET", requestUrl,
            "{\"query\":{\"pages\":{\"1\":{\"imageinfo\":[{\"thumburl\":\"https://upload.wikimedia.org/full.jpg\","
            "\"extmetadata\":{\"ImageDescription\":{\"value\":\"Full caption\"}}}]}}}}");
        SettingsState settings;
        ImageSelectionState selection(settings);
        selection.selectImage("previous.jpg", "Previous caption");
        QSignalSpy urlSpy(&selection, &ImageSelectionState::currentImageUrlChanged);
        network.deferNextReply();
        selection.selectGalleryImage({"https://upload.wikimedia.org/thumb.jpg"}, {"Thumbnail caption"}, {title}, 0);
        QCOMPARE(selection.currentImageUrl(), QString("previous.jpg"));
        QCOMPARE(selection.currentImageDescription(), QString("Previous caption"));
        QCOMPARE(urlSpy.count(), 0);
        QCOMPARE(network.requestCount(), 1);
        auto *client = selection.findChild<WikipediaPageClient *>();
        QVERIFY(client);
        emit client->imageDetailsReceived("File:Stale.jpg", "stale.jpg", "stale caption");
        QCOMPARE(selection.currentImageUrl(), QString("previous.jpg"));
        selection.selectGalleryImage({"https://upload.wikimedia.org/thumb.jpg"}, {"Thumbnail caption"}, {title}, 0);
        QCOMPARE(network.requestCount(), 1);
        network.completeNextReply();
        QTRY_COMPARE(selection.currentImageUrl(), QString("https://upload.wikimedia.org/full.jpg"));
        QCOMPARE(selection.currentImageDescription(), QString("Full caption"));
        QCOMPARE(urlSpy.count(), 1);
    }

    void detailsFailureFallsBackToThumbnail() {
        NetworkFixtureController network;
        connect(&network, &NetworkFixtureController::unexpectedRequest, this,
                [](const QString &request) { qFatal("%s", qPrintable(request)); });
        WikipediaNetwork::installNetworkAccessManagerFactory(&network);
        const QString title = "File:Broken.jpg";
        QUrl requestUrl("https://en.wikipedia.org/w/api.php");
        QUrlQuery query;
        query.addQueryItem("action", "query");
        query.addQueryItem("format", "json");
        query.addQueryItem("prop", "imageinfo");
        query.addQueryItem("iiprop", "url|extmetadata|mime");
        query.addQueryItem("iiurlheight", "2048");
        query.addQueryItem("iiextmetadatafilter", "ImageDescription");
        query.addQueryItem("iiurlwidth", "2048");
        query.addQueryItem("titles", title);
        requestUrl.setQuery(query);
        network.addFixture("GET", requestUrl, "{}", "application/json", 500,
                             QNetworkReply::InternalServerError, "request failed");
        SettingsState settings;
        ImageSelectionState selection(settings);
        selection.selectGalleryImage({"thumb.jpg"}, {"caption"}, {title}, 0);
        QTRY_COMPARE(selection.currentImageUrl(), QString("thumb.jpg"));
        QCOMPARE(selection.currentImageDescription(), QString("caption"));
    }

    void missingDetailsFallbackToThumbnail() {
        NetworkFixtureController network;
        connect(&network, &NetworkFixtureController::unexpectedRequest, this,
                [](const QString &request) { qFatal("%s", qPrintable(request)); });
        WikipediaNetwork::installNetworkAccessManagerFactory(&network);
        const QString title = "File:Missing.jpg";
        QUrl requestUrl("https://en.wikipedia.org/w/api.php");
        QUrlQuery query;
        query.addQueryItem("action", "query");
        query.addQueryItem("format", "json");
        query.addQueryItem("prop", "imageinfo");
        query.addQueryItem("iiprop", "url|extmetadata|mime");
        query.addQueryItem("iiurlheight", "2048");
        query.addQueryItem("iiextmetadatafilter", "ImageDescription");
        query.addQueryItem("iiurlwidth", "2048");
        query.addQueryItem("titles", title);
        requestUrl.setQuery(query);
        network.addFixture("GET", requestUrl, "{\"query\":{\"pages\":{\"-1\":{\"missing\":\"\"}}}}");
        SettingsState settings;
        ImageSelectionState selection(settings);
        selection.selectImage("previous.jpg", "Previous caption");
        selection.selectGalleryImage({"thumb.jpg"}, {"caption"}, {title}, 0);
        QTRY_COMPARE(selection.currentImageUrl(), QString("thumb.jpg"));
        QCOMPARE(selection.currentImageDescription(), QString("caption"));
    }

    void teardownAndRecreation() {
        QVERIFY(ImageSelectionState::instance().isNull());
        {
            SettingsState settings;
            ImageSelectionState selection(settings);
            QCOMPARE(ImageSelectionState::instance().data(), &selection);
            selection.selectImage("image.png", "caption");
        }
        QVERIFY(ImageSelectionState::instance().isNull());
        SettingsState settings;
        ImageSelectionState selection(settings);
        QVERIFY(selection.currentImageUrl().isEmpty());
        QVERIFY(selection.currentImageDescription().isEmpty());
    }
};

QTEST_GUILESS_MAIN(ImageSelectionStateTest)
#include "ImageSelectionStateTest.moc"
