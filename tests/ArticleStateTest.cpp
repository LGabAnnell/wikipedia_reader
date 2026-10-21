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
                    "&pageids=42");
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
        QCOMPARE(m_history->history().first().language, QString("en"));
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

    void missingTitleResolutionStopsLoadingAndAllowsRetry() {
        SettingsState settings;
        ArticleState article(*m_history, settings);
        const QUrl resolveUrl("https://en.wikipedia.org/w/api.php?action=query&format=json&titles=Missing%20article&prop=pageprops%7Cpageids");
        m_network.addFixture("GET", resolveUrl,
            R"({"query":{"pages":{"-1":{"missing":"","title":"Missing article"}}}})");
        QSignalSpy historySpy(m_history, &HistoryState::historyChanged);
        m_network.deferNextReply();

        article.loadArticleByTitle("Missing article");
        QVERIFY(article.isLoading());
        QCOMPARE(m_history->history().size(), 0);
        m_network.completeNextReply();

        QTRY_VERIFY(!article.isLoading());
        QVERIFY(!article.errorMessage().isEmpty());
        QCOMPARE(m_network.requestCount(), 1);
        QCOMPARE(historySpy.count(), 0);
        QCOMPARE(m_history->history().size(), 0);

        const QUrl retryResolveUrl("https://en.wikipedia.org/w/api.php?action=query&format=json&titles=Article&prop=pageprops%7Cpageids");
        m_network.addFixture("GET", retryResolveUrl,
            R"({"query":{"pages":{"42":{"pageid":42,"title":"Article"}}}})");
        addArticle();
        article.loadArticleByTitle("Article");
        QVERIFY(article.isLoading());
        QVERIFY(article.errorMessage().isEmpty());
        QTRY_VERIFY(!article.isLoading());
        QCOMPARE(article.currentPageId(), 42);
        QCOMPARE(article.currentPageTitle(), QString("Article"));
        QCOMPARE(historySpy.count(), 1);
        QCOMPARE(m_history->history().size(), 1);
        QCOMPARE(m_network.requestCount(), 4);
    }

    void loadArticleByTitleClearsCurrentArticle() {
        SettingsState settings;
        ArticleState article(*m_history, settings);
        article.setCurrentPage(cachedArticle());
        QCOMPARE(article.currentPageTitle(), QString("Cached article"));
        m_network.addFixture("GET",
            QUrl("https://en.wikipedia.org/w/api.php?action=query&format=json&titles=Cached%20article&prop=pageprops%7Cpageids"),
            R"({"query":{"pages":{"42":{"pageid":42,"title":"Cached article"}}}})");
        m_network.deferNextReply();
        article.loadArticleByTitle("Cached article");
        // The previous article is dropped immediately so a newly pushed
        // article view does not flash it while the linked article loads.
        QVERIFY(article.isLoading());
        QVERIFY(article.currentPageTitle().isEmpty());
        QVERIFY(article.currentPageExtract().isEmpty());
        QCOMPARE(article.currentPageId(), 0);
        m_network.completeNextReply();
        QTRY_VERIFY(!article.isLoading());
        QCOMPARE(article.currentPageTitle(), QString("Cached article"));
        QCOMPARE(article.currentPageExtract(), QString("<p>Cached HTML</p>"));
        QCOMPARE(m_network.requestCount(), 1);
    }

    void languageChangeClearsCurrentArticleAndCache() {
        SettingsState settings;
        ArticleState article(*m_history, settings);
        article.setCurrentPage(cachedArticle());
        addArticle("fr", "French article");
        settings.setLanguage("fr");
        QCOMPARE(m_network.requestCount(), 0);
        QVERIFY(article.currentPageTitle().isEmpty());
        QVERIFY(article.currentPageExtract().isEmpty());
        QCOMPARE(article.currentPageId(), 0);
        QVERIFY(article.errorMessage().isEmpty());
        QVERIFY(!article.isLoading());
        article.loadArticleByPageId(42);
        QVERIFY(article.isLoading());
        QTRY_VERIFY(!article.isLoading());
        QCOMPARE(article.currentPageTitle(), QString("French article"));
        QCOMPARE(m_network.requestCount(), 2);
        QCOMPARE(m_network.requests().first().toMap().value("host").toString(), QString("fr.wikipedia.org"));
    }

    void obsoleteArticleReplyCannotReplaceNewLanguage() {
        SettingsState settings;
        ArticleState article(*m_history, settings);
        addArticle("fr", "French article");
        m_network.addFixture("GET", articleUrl("en"),
            R"({"query":{"pages":{"42":{"pageid":42,"title":"Old article","extract":"old","images":[]}}}})");
        m_network.deferNextReply(true);
        article.loadArticleByPageId(42);
        QVERIFY(article.isLoading());
        QCOMPARE(m_network.pendingReplyCount(), 1);
        settings.setLanguage("fr");
        QVERIFY(!article.isLoading());
        QCOMPARE(m_network.pendingReplyCount(), 1);
        m_network.completeReply(articleUrl("en"));
        QCoreApplication::processEvents();
        QVERIFY(article.currentPageTitle().isEmpty());
        QVERIFY(article.errorMessage().isEmpty());
        QVERIFY(!article.isLoading());
        QCOMPARE(m_history->history().size(), 0);
        QCOMPARE(m_network.requestCount(), 1);
    }

    void obsoleteTitleResolutionCannotStartPageRequest() {
        SettingsState settings;
        ArticleState article(*m_history, settings);
        const QUrl resolveUrl("https://en.wikipedia.org/w/api.php?action=query&format=json&titles=Old%20title&prop=pageprops%7Cpageids");
        m_network.addFixture("GET", resolveUrl,
            R"({"query":{"pages":{"42":{"pageid":42,"title":"Old title"}}}})");
        m_network.deferNextReply(true);
        article.loadArticleByTitle("Old title");
        settings.setLanguage("fr");
        m_network.completeReply(resolveUrl);
        QCoreApplication::processEvents();
        QCOMPARE(m_network.requestCount(), 1);
        QVERIFY(article.currentPageTitle().isEmpty());
        QVERIFY(!article.isLoading());
        QVERIFY(article.errorMessage().isEmpty());
    }

    void cachedSelectionSupersedesPendingArticle() {
        SettingsState settings;
        ArticleState article(*m_history, settings);
        article.setCurrentPage(cachedArticle());
        const QUrl obsoleteUrl("https://en.wikipedia.org/w/api.php?action=query&format=json&pageids=43");
        m_network.addFixture("GET", obsoleteUrl,
            R"({"query":{"pages":{"43":{"pageid":43,"title":"Obsolete","extract":"","images":[]}}}})");
        m_network.deferNextReply(true);
        article.loadArticleByPageId(43);
        QVERIFY(article.isLoading());
        article.loadArticleByPageId(42);
        QVERIFY(!article.isLoading());
        QCOMPARE(article.currentPageTitle(), QString("Cached article"));
        m_network.completeReply(obsoleteUrl);
        QCoreApplication::processEvents();
        QCOMPARE(article.currentPageTitle(), QString("Cached article"));
        QVERIFY(article.errorMessage().isEmpty());
        QCOMPARE(m_history->history().size(), 1);
        QCOMPARE(m_network.requestCount(), 1);
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

    void invalidParseResponseDoesNotCacheOrRecordHistoryAndCanRetry() {
        SettingsState settings;
        ArticleState article(*m_history, settings);
        m_network.addFixture("GET", articleUrl(),
            R"({"query":{"pages":{"42":{"pageid":42,"title":"Article","extract":"Plain text","images":[]}}}})");
        const QUrl parseUrl("https://en.wikipedia.org/w/api.php?action=parse&disableeditsection=true"
                            "&format=json&formatversion=2&pageid=42&prop=text");
        m_network.addFixture("GET", parseUrl, R"({"error":{"code":"badtoken","info":"Invalid token"}})");

        article.loadArticleByPageId(42);

        QTRY_VERIFY(!article.isLoading());
        QCOMPARE(article.currentPageId(), 0);
        QVERIFY(article.currentPageTitle().isEmpty());
        QVERIFY(!article.errorMessage().isEmpty());
        QVERIFY(m_history->history().isEmpty());
        QCOMPARE(m_network.requestCount(), 2);

        addArticle();
        article.loadArticleByPageId(42);

        QTRY_VERIFY(!article.isLoading());
        QCOMPARE(article.currentPageId(), 42);
        QVERIFY(article.errorMessage().isEmpty());
        QCOMPARE(m_history->history().size(), 1);
        QCOMPARE(m_network.requestCount(), 4);
    }


    void historyIdentityAndBlankLanguageValidation() {
        m_history->addToHistory("English", 42, "en");
        m_history->addToHistory("French", 42, "fr");
        m_history->addToHistory("Updated English", 42, "en");
        m_history->addToHistory("Unknown", 99, "  ");
        const auto items = m_history->history();
        QCOMPARE(items.size(), 2);
        QCOMPARE(items[0].language, QString("en"));
        QCOMPARE(items[0].title, QString("Updated English"));
        QCOMPARE(items[1].language, QString("fr"));
        QCOMPARE(items[1].title, QString("French"));
        delete m_history;
        m_history = new HistoryState(this);
        const auto restored = m_history->history();
        QCOMPARE(restored.size(), 2);
        QCOMPARE(restored[0].language, QString("en"));
        QCOMPARE(restored[0].title, QString("Updated English"));
        QCOMPARE(restored[1].language, QString("fr"));
        QCOMPARE(restored[1].title, QString("French"));
    }

    void historyRestoresEditionBeforeLoadingAndRecordsCacheHits() {
        SettingsState settings;
        ArticleState article(*m_history, settings);
        article.setCurrentPage(cachedArticle());
        addArticle("fr", "French article");
        QVERIFY(article.loadArticleFromHistory("fr", 42));
        QCOMPARE(settings.language(), QString("fr"));
        QTRY_VERIFY(!article.isLoading());
        QCOMPARE(article.currentPageTitle(), QString("French article"));
        QCOMPARE(m_history->history().size(), 2);
        QCOMPARE(m_history->history()[0].language, QString("fr"));
        QCOMPARE(m_history->history()[1].language, QString("en"));
        QCOMPARE(m_network.requests().first().toMap().value("host").toString(), QString("fr.wikipedia.org"));
        QSignalSpy visits(m_history, &HistoryState::historyChanged);
        QVERIFY(article.loadArticleFromHistory("fr", 42));
        QCOMPARE(visits.count(), 1);
        QCOMPARE(m_network.requestCount(), 2);
        QCOMPARE(m_history->history().size(), 2);
        QCOMPARE(m_history->history()[0].language, QString("fr"));
    }

    void invalidHistoryEntryDoesNotChangeState() {
        SettingsState settings;
        ArticleState article(*m_history, settings);
        article.setCurrentPage(cachedArticle());
        article.setErrorMessage("Existing error");
        QSignalSpy pageChanges(&article, &ArticleState::currentPageChanged);
        QSignalSpy languageChanges(&settings, &SettingsState::languageChanged);
        QVERIFY(!article.loadArticleFromHistory("", 42));
        QVERIFY(!article.loadArticleFromHistory("  ", 42));
        QVERIFY(!article.loadArticleFromHistory("fr", 0));
        QVERIFY(!article.loadArticleFromHistory("fr", -42));
        QCOMPARE(settings.language(), QString("en"));
        QCOMPARE(article.currentPageTitle(), QString("Cached article"));
        QCOMPARE(article.errorMessage(), QString("Existing error"));
        QCOMPARE(pageChanges.count(), 0);
        QCOMPARE(languageChanges.count(), 0);
        QCOMPARE(m_network.requestCount(), 0);
    }

    void historyLoadFailureKeepsRestoredEdition() {
        SettingsState settings;
        ArticleState article(*m_history, settings);
        m_network.addFixture("GET", articleUrl("fr"), "failure", "text/plain", 500);
        QVERIFY(article.loadArticleFromHistory("fr", 42));
        QTRY_VERIFY(!article.isLoading());
        QCOMPARE(settings.language(), QString("fr"));
        QCOMPARE(article.errorMessage(), QString("HTTP 500"));
        QVERIFY(m_history->history().isEmpty());
    }

    void completionRecordsEditionBeforeObserversChangeLanguage() {
        SettingsState settings;
        ArticleState article(*m_history, settings);
        addArticle();
        connect(&article, &ArticleState::isLoadingChanged, &settings, [&]() {
            if (!article.isLoading())
                settings.setLanguage("fr");
        });
        article.loadArticleByPageId(42);
        QTRY_COMPARE(settings.language(), QString("fr"));
        QCOMPARE(m_history->history().size(), 1);
        QCOMPARE(m_history->history()[0].language, QString("en"));
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
