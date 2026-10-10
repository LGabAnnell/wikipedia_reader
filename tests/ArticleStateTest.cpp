#include "ArticleState.h"
#include "support/FakeNetworkAccessManager.h"
#include "wikipedia_network_access_manager.h"
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QTest>

class ArticleStateTest : public QObject {
    Q_OBJECT

  private:
    QTemporaryDir m_dataDirectory;
    NetworkFixtureController m_network;
    HistoryState *m_history = nullptr;

    QUrl articleUrl(const QString &language = "en") const {
        return QUrl("https://" + language + ".wikipedia.org/w/api.php?action=query&format=json"
                    "&prop=extracts%7Cimages&pageids=42&explaintext=1&imlimit=50");
    }

    void addArticle(const QString &language = "en", const QString &title = "Article") {
        m_network.addFixture("GET", articleUrl(language),
            QString(R"({"query":{"pages":{"42":{"pageid":42,"title":"%1","extract":"Plain text","images":[]}}}})")
                .arg(title).toUtf8());
        m_network.addFixture("GET",
            QUrl("https://" + language + ".wikipedia.org/w/api.php?action=parse&disableeditsection=true"
                 "&format=json&formatversion=2&pageid=42&prop=text"),
            R"({"parse":{"text":"<p>Article HTML</p>"}})");
    }

    page cachedArticle() const {
        page article{};
        article.title = "Cached article";
        article.extract = "<p>Cached HTML</p>";
        article.pageid = 42;
        return article;
    }

  private slots:
    void initTestCase() {
        QVERIFY(m_dataDirectory.isValid());
        qputenv("XDG_DATA_HOME", m_dataDirectory.path().toUtf8());
        QCoreApplication::setApplicationName("ArticleStateTest");
        m_history = new HistoryState(this);
        connect(&m_network, &NetworkFixtureController::unexpectedRequest, this,
                [](const QString &request) { qFatal("%s", qPrintable(request)); });
        WikipediaNetwork::installNetworkAccessManagerFactory(&m_network);
    }

    void cleanupTestCase() {
        delete m_history;
        m_history = nullptr;
        WikipediaNetwork::installNetworkAccessManagerFactory(nullptr);
    }

    void init() {
        m_history->clearHistory();
        m_network.clearRequests();
        QVERIFY(ArticleState::instance().isNull());
    }

    void cleanup() {
        QCOMPARE(m_network.pendingReplyCount(), 0);
        QVERIFY(ArticleState::instance().isNull());
    }

    void initialStateAndNotifications() {
        SettingsState settings;
        ArticleState article(*m_history, settings);
        QCOMPARE(ArticleState::instance().data(), &article);
        QCOMPARE(article.currentPageId(), 0);
        QVERIFY(article.currentPageTitle().isEmpty());
        QVERIFY(article.currentPageExtract().isEmpty());
        QVERIFY(article.currentPageImageUrls().isEmpty());
        QVERIFY(!article.isLoading());
        QVERIFY(article.errorMessage().isEmpty());
        QSignalSpy pageSpy(&article, &ArticleState::currentPageChanged);
        QSignalSpy loadingSpy(&article, &ArticleState::isLoadingChanged);
        QSignalSpy errorSpy(&article, &ArticleState::errorMessageChanged);
        article.setCurrentPageFromData("Preview", "<p>Preview</p>", "");
        QCOMPARE(article.currentPageTitle(), QString("Preview"));
        QCOMPARE(article.currentPageExtract(), QString("<p>Preview</p>"));
        QCOMPARE(article.currentPageId(), 0);
        QCOMPARE(pageSpy.count(), 1);
        QVERIFY(m_history->history().isEmpty());
        article.setIsLoading(true);
        article.setIsLoading(true);
        QCOMPARE(loadingSpy.count(), 1);
        article.setIsLoading(false);
        QCOMPARE(loadingSpy.count(), 2);
        article.setErrorMessage("failure");
        article.setErrorMessage("failure");
        QCOMPARE(errorSpy.count(), 1);
        article.clearErrorMessage();
        QVERIFY(article.errorMessage().isEmpty());
        QCOMPARE(errorSpy.count(), 2);
        QCOMPARE(m_network.requestCount(), 0);
    }

    void networkLoadCachesHtmlAndRecordsEveryVisit() {
        SettingsState settings;
        ArticleState article(*m_history, settings);
        addArticle();
        QSignalSpy historySpy(m_history, &HistoryState::historyChanged);
        m_network.deferNextReply();
        article.loadArticleByPageId(42);
        QVERIFY(article.isLoading());
        QVERIFY(m_history->history().isEmpty());
        QCOMPARE(m_network.pendingReplyCount(), 1);
        m_network.completeNextReply();
        QTRY_VERIFY(!article.isLoading());
        QCOMPARE(article.currentPageId(), 42);
        QVERIFY(article.currentPageExtract().contains("<p>Article HTML</p>"));
        QCOMPARE(historySpy.count(), 1);
        QCOMPARE(m_history->history().first().pageId, 42);
        const QString html = article.currentPageExtract();
        article.setErrorMessage("old failure");
        article.loadArticleByPageId(42);
        QVERIFY(!article.isLoading());
        QVERIFY(article.errorMessage().isEmpty());
        QCOMPARE(article.currentPageExtract(), html);
        QCOMPARE(m_network.requestCount(), 2);
        QCOMPARE(historySpy.count(), 2);
        QCOMPARE(m_history->history().size(), 1);
    }

    void titleResolutionUsesCachedArticle() {
        SettingsState settings;
        ArticleState article(*m_history, settings);
        article.setCurrentPage(cachedArticle());
        m_network.addFixture("GET",
            QUrl("https://en.wikipedia.org/w/api.php?action=query&format=json&titles=Cached%20article&prop=pageprops%7Cpageids"),
            R"({"query":{"pages":{"42":{"pageid":42,"title":"Cached article"}}}})");
        QSignalSpy historySpy(m_history, &HistoryState::historyChanged);
        article.loadArticleByTitle("Cached article");
        QVERIFY(article.isLoading());
        QTRY_VERIFY(!article.isLoading());
        QCOMPARE(article.currentPageExtract(), QString("<p>Cached HTML</p>"));
        QCOMPARE(m_network.requestCount(), 1);
        QCOMPARE(historySpy.count(), 1);
    }

    void languageChangeInvalidatesCacheWithoutReloading() {
        SettingsState settings;
        ArticleState article(*m_history, settings);
        article.setCurrentPage(cachedArticle());
        addArticle("fr", "French article");
        settings.setLanguage("fr");
        QCOMPARE(m_network.requestCount(), 0);
        QCOMPARE(article.currentPageTitle(), QString("Cached article"));
        article.loadArticleByPageId(42);
        QVERIFY(article.isLoading());
        QTRY_VERIFY(!article.isLoading());
        QCOMPARE(article.currentPageTitle(), QString("French article"));
        QCOMPARE(m_network.requestCount(), 2);
        QCOMPARE(m_network.requests().first().toMap().value("host").toString(), QString("fr.wikipedia.org"));
    }

    void clientStartsFromCurrentSettings() {
        SettingsState settings;
        settings.setLanguage("de");
        ArticleState article(*m_history, settings);
        addArticle("de", "German article");
        article.loadArticleByPageId(42);
        QTRY_VERIFY(!article.isLoading());
        QCOMPARE(article.currentPageTitle(), QString("German article"));
        QCOMPARE(m_network.requestCount(), 2);
    }

    void failureStopsLoadingAndRetryClearsError() {
        SettingsState settings;
        ArticleState article(*m_history, settings);
        m_network.addFixture("GET", articleUrl(), "failure", "text/plain", 500);
        article.loadArticleByPageId(42);
        QTRY_VERIFY(!article.isLoading());
        QCOMPARE(article.errorMessage(), QString("HTTP 500"));
        QVERIFY(m_history->history().isEmpty());
        addArticle();
        m_network.deferNextReply();
        article.loadArticleByPageId(42);
        QVERIFY(article.isLoading());
        QVERIFY(article.errorMessage().isEmpty());
        m_network.completeNextReply();
        QTRY_VERIFY(!article.isLoading());
        QCOMPARE(article.currentPageId(), 42);
    }

    void teardownAndRecreation() {
        SettingsState settings;
        {
            ArticleState article(*m_history, settings);
            article.setCurrentPage(cachedArticle());
        }
        QVERIFY(ArticleState::instance().isNull());
        ArticleState article(*m_history, settings);
        QCOMPARE(ArticleState::instance().data(), &article);
        QCOMPARE(article.currentPageId(), 0);
        QVERIFY(article.currentPageTitle().isEmpty());
        // A fresh state must have a fresh cache.
        addArticle();
        article.loadArticleByPageId(42);
        QTRY_VERIFY(!article.isLoading());
        QCOMPARE(m_network.requestCount(), 2);
    }
};

QTEST_GUILESS_MAIN(ArticleStateTest)
#include "ArticleStateTest.moc"
