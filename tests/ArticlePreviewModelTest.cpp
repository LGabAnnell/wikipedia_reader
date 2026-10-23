#include <QtTest/QtTest>
#include "ArticlePreviewModel.h"
#include "SettingsState.h"
#include "wikipedia_network_access_manager.h"
#include "support/FakeNetworkAccessManager.h"

class ArticlePreviewModelTest final : public QObject {
    Q_OBJECT
  private:
    NetworkFixtureController m_network;
    SettingsState m_settings;
    QByteArray body(const QString &title, int pageId, int namespaceId = 0) const {
        return QStringLiteral("{\"title\":\"%1\",\"pageid\":%2,\"namespace\":{\"id\":%3},\"extract\":\"Intro\",\"extract_html\":\"<p><b>%1</b></p>\"}")
            .arg(title).arg(pageId).arg(namespaceId).toUtf8();
    }
    QUrl url(const QString &title, const QString &language = "en") const {
        return QUrl(QStringLiteral("https://%1.wikipedia.org/api/rest_v1/page/summary/%2")
                        .arg(language, QString::fromLatin1(QUrl::toPercentEncoding(title))));
    }
  private slots:
    void initTestCase() {
        connect(&m_network, &NetworkFixtureController::unexpectedRequest, this,
                [](const QString &request) { qFatal("%s", qPrintable(request)); });
        WikipediaNetwork::installNetworkAccessManagerFactory(&m_network);
    }
    void cleanupTestCase() { WikipediaNetwork::installNetworkAccessManagerFactory(nullptr); }
    void init() {
        m_settings.setLanguage("en");
        m_network.clearRequests();
    }
    void cleanup() { QCOMPARE(m_network.pendingReplyCount(), 0); }

    void successfulSummariesAreCachedPerModel() {
        m_network.addFixture("GET", url("Cache page"), body("Cache page", 1));
        ArticlePreviewModel model;
        model.requestPreview("Cache page");
        QTRY_COMPARE(model.status(), ArticlePreviewModel::Ready);
        model.clearPreview();
        model.requestPreview("Cache page");
        QCOMPARE(model.status(), ArticlePreviewModel::Ready);
        QCOMPARE(m_network.requestCount(), 1);
        QCOMPARE(model.pageId(), 1);
    }

    void preservesTitleCaseInCacheIdentity() {
        m_network.addFixture("GET", url("Paris"), body("Paris", 12));
        m_network.addFixture("GET", url("paris"), body("paris", 13));
        ArticlePreviewModel model;
        model.requestPreview("Paris");
        QTRY_COMPARE(model.status(), ArticlePreviewModel::Ready);
        QCOMPARE(model.pageId(), 12);
        model.requestPreview("paris");
        QTRY_COMPARE(model.status(), ArticlePreviewModel::Ready);
        QCOMPARE(model.title(), QString("paris"));
        QCOMPARE(model.pageId(), 13);
        QCOMPARE(m_network.requestCount(), 2);
    }

    void cachedSummaryExpiresAfterFiveMinutes() {
        QDateTime now = QDateTime::fromSecsSinceEpoch(1000, QTimeZone::UTC);
        const QString title = "Expiring page";
        m_network.addFixture("GET", url(title), body(title, 2));
        ArticlePreviewModel model([&now]() { return now; }, nullptr);
        model.requestPreview(title);
        QTRY_COMPARE(model.status(), ArticlePreviewModel::Ready);
        model.clearPreview();
        now = now.addSecs(ArticlePreviewModel::CacheTtlSeconds + 1);
        model.requestPreview(title);
        QTRY_COMPARE(model.status(), ArticlePreviewModel::Ready);
        QCOMPARE(m_network.requestCount(), 2);
    }

    void nonArticleSummariesAreCachedUntilExpiry() {
        QDateTime now = QDateTime::fromSecsSinceEpoch(1000, QTimeZone::UTC);
        const QString title = "Category:Books";
        m_network.addFixture("GET", url(title), body(title, 14, 14));
        ArticlePreviewModel model([&now]() { return now; }, nullptr);
        QSignalSpy readySpy(&model, &ArticlePreviewModel::summaryReady);
        model.requestPreview(title);
        QTRY_COMPARE(model.status(), ArticlePreviewModel::Unavailable);
        QVERIFY(model.suppressed());

        model.requestPreview(title);
        QCOMPARE(model.status(), ArticlePreviewModel::Unavailable);
        QCOMPARE(m_network.requestCount(), 1);

        model.clearPreview();
        now = now.addSecs(ArticlePreviewModel::CacheTtlSeconds - 1);
        model.requestPreview(title);
        QCOMPARE(model.status(), ArticlePreviewModel::Unavailable);
        QVERIFY(model.suppressed());
        QVERIFY(model.title().isEmpty());
        QVERIFY(model.summaryHtml().isEmpty());
        QVERIFY(model.thumbnailUrl().isEmpty());
        QCOMPARE(model.pageId(), 0);
        QCOMPARE(m_network.requestCount(), 1);

        model.clearPreview();
        now = now.addSecs(1);
        model.requestPreview(title);
        QCOMPARE(model.status(), ArticlePreviewModel::Loading);
        QTRY_COMPARE(model.status(), ArticlePreviewModel::Unavailable);
        QVERIFY(model.suppressed());
        QCOMPARE(m_network.requestCount(), 2);
        QCOMPARE(readySpy.count(), 0);
    }

    void languageTransitionClearsSuppressedCache() {
        const QString title = "Category:Books";
        m_network.addFixture("GET", url(title, "en"), body(title, 14, 14));
        m_network.addFixture("GET", url(title, "fr"), body(title, 15));
        ArticlePreviewModel model;
        model.requestPreview(title);
        QTRY_COMPARE(model.status(), ArticlePreviewModel::Unavailable);
        QVERIFY(model.suppressed());

        m_settings.setLanguage("fr");
        QCOMPARE(model.status(), ArticlePreviewModel::Idle);
        QVERIFY(!model.suppressed());
        model.requestPreview(title);
        QTRY_COMPARE(model.status(), ArticlePreviewModel::Ready);
        QCOMPARE(model.pageId(), 15);
        QVERIFY(!model.suppressed());

        m_settings.setLanguage("en");
        model.requestPreview(title);
        QCOMPARE(model.status(), ArticlePreviewModel::Loading);
        QTRY_COMPARE(model.status(), ArticlePreviewModel::Unavailable);
        QVERIFY(model.suppressed());
        QCOMPARE(m_network.requestCount(), 3);
    }

    void leastRecentlyUsedEntryIsEvictedAfterFiftyPages() {
        ArticlePreviewModel model;
        for (int i = 0; i <= 50; ++i) {
            const QString title = QStringLiteral("Page %1").arg(i);
            m_network.addFixture("GET", url(title), body(title, i + 1));
            model.requestPreview(title);
            QTRY_COMPARE(model.status(), ArticlePreviewModel::Ready);
        }
        QCOMPARE(m_network.requestCount(), 51);
        model.requestPreview("Page 0");
        QTRY_COMPARE(model.status(), ArticlePreviewModel::Ready);
        QCOMPARE(m_network.requestCount(), 52);
    }

    void languageTransitionClearsCacheAndPendingPreview() {
        const QString title = "Edition page";
        m_network.addFixture("GET", url(title, "en"), body(title, 4));
        m_network.addFixture("GET", url(title, "fr"), body(title, 5));
        ArticlePreviewModel model;
        model.requestPreview(title);
        QTRY_COMPARE(model.status(), ArticlePreviewModel::Ready);
        m_settings.setLanguage("fr");
        QCOMPARE(model.status(), ArticlePreviewModel::Idle);
        model.requestPreview(title);
        QTRY_COMPARE(model.status(), ArticlePreviewModel::Ready);
        QCOMPARE(model.pageId(), 5);
        QCOMPARE(m_network.requestCount(), 2);
        m_settings.setLanguage("en");
    }

    void aNewTargetSupersedesAnOlderDeferredReply() {
        m_network.addFixture("GET", url("Old page"), body("Old page", 7));
        m_network.addFixture("GET", url("New page"), body("New page", 8));
        ArticlePreviewModel model;
        m_network.deferNextReply(true);
        model.requestPreview("Old page");
        QTRY_COMPARE(m_network.pendingReplyCount(), 1);
        model.requestPreview("New page");
        QTRY_COMPARE(model.status(), ArticlePreviewModel::Ready);
        QCOMPARE(model.title(), QString("New page"));
        QCOMPARE(model.pageId(), 8);
        m_network.completeNextReply();
        QTest::qWait(10);
        QCOMPARE(model.title(), QString("New page"));
        QCOMPARE(model.pageId(), 8);
    }

    void pendingRequestCannotReplaceANewerCachedTarget() {
        m_network.addFixture("GET", url("Cached page"), body("Cached page", 10));
        m_network.addFixture("GET", url("Old pending page"), body("Old pending page", 11));
        ArticlePreviewModel model;
        model.requestPreview("Cached page");
        QTRY_COMPARE(model.status(), ArticlePreviewModel::Ready);
        model.clearPreview();
        m_network.deferNextReply(true);
        model.requestPreview("Old pending page");
        QTRY_COMPARE(m_network.pendingReplyCount(), 1);
        model.requestPreview("Cached page");
        QCOMPARE(model.status(), ArticlePreviewModel::Ready);
        QCOMPARE(model.title(), QString("Cached page"));
        m_network.completeNextReply();
        QTest::qWait(10);
        QCOMPARE(model.title(), QString("Cached page"));
        QCOMPARE(model.pageId(), 10);
    }
};
QTEST_MAIN(ArticlePreviewModelTest)
#include "ArticlePreviewModelTest.moc"

