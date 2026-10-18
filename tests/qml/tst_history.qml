import QtQuick
import QtTest
import wikipedia_qt
import wikipedia_qt.History

Item {
    id: root
    width: 800
    height: 600

    Component {
        id: legacyHistoryComponent
        History { history: [historyTests.unknownEntry] }
    }

    Component { id: historyComponent; History {} }
    Component { id: unavailableComponent; History { databaseReady: false } }

    TestCase {
        id: historyTests
        property history_item unknownEntry
        name: "ArticleHistoryBehavior"
        when: windowShown

        readonly property int firstArticleId: 701
        readonly property int secondArticleId: 702

        function pageQueryUrl(pageId) {
            return "https://en.wikipedia.org/w/api.php?action=query&format=json"
                    + "&prop=extracts%7Cimages&pageids=" + pageId
                    + "&explaintext=1&imlimit=50"
        }

        function parseUrl(pageId) {
            return "https://en.wikipedia.org/w/api.php?action=parse&disableeditsection=true"
                    + "&format=json&formatversion=2&pageid=" + pageId + "&prop=text"
        }

        function configureArticleFixture(pageId, title) {
            var pages = {}
            pages[pageId] = {
                "pageid": pageId,
                "title": title,
                "extract": title + " extract",
                "images": []
            }
            networkFixtures.addFixture("GET", pageQueryUrl(pageId),
                                       JSON.stringify({"query": {"pages": pages}}))
            networkFixtures.addFixture("GET", parseUrl(pageId),
                                       JSON.stringify({"parse": {
                                           "title": title,
                                           "pageid": pageId,
                                           "text": "<p>" + title + "</p>"
                                       }}))
        }

        function loadArticle(pageId) {
            ArticleState.loadArticleByPageId(pageId)
            tryCompare(ArticleState, "currentPageId", pageId)
            tryCompare(ArticleState, "isLoading", false)
        }

        function init() {
            testSupport.clearRequests()
            testSupport.clearQmlWarnings()
            HistoryState.clearHistory()
            ImageSelectionState.selectImage("", "")
            configureArticleFixture(firstArticleId, "First history article")
            configureArticleFixture(secondArticleId, "Second history article")
        }

        function cleanup() {
            verify(testSupport.setHistoryClearFailure(false))
            while (networkFixtures.pendingReplyCount > 0)
                networkFixtures.completeNextReply()
            ImageSelectionState.selectImage("", "")
            HistoryState.clearHistory()
            compare(testSupport.qmlWarningCount, 0, testSupport.qmlWarnings.join("\n"))
        }

        function test_failed_clear_feedback() {
            HistoryState.addToHistory("Retained", 1, "en")
            const view = createTemporaryObject(historyComponent, root)
            verify(testSupport.setHistoryClearFailure(true))
            mouseClick(findChild(view, "clearHistoryButton"))
            tryCompare(findChild(view, "historyErrorDialog"), "visible", true)
            verify(findChild(view, "historyErrorText").text.indexOf("History clear failed") >= 0)
            compare(HistoryState.history.length, 1)
            compare(findChild(view, "clearHistoryButton").enabled, true)
            verify(testSupport.setHistoryClearFailure(false))
        }

        function test_error_before_creation() {
            HistoryState.addToHistory("Retained", 1, "en")
            verify(testSupport.setHistoryClearFailure(true))
            HistoryState.clearHistory()
            const view = createTemporaryObject(historyComponent, root)
            tryCompare(findChild(view, "historyErrorDialog"), "visible", true)
            verify(findChild(view, "historyErrorText").text.indexOf("History clear failed") >= 0)
            verify(testSupport.setHistoryClearFailure(false))
        }

        function test_unavailable() {
            const view = createTemporaryObject(unavailableComponent, root)
            compare(findChild(view, "historyUnavailableLabel").text, "Browsing history is unavailable")
            compare(findChild(view, "historyUnavailableLabel").visible, true)
        }

        function test_unknown_edition_is_visible_but_cannot_open() {
            HistoryState.addToHistory("Legacy title", 701, "en")
            unknownEntry = HistoryState.history[0]
            unknownEntry.language = ""
            const view = createTemporaryObject(legacyHistoryComponent, root)
            verify(view !== null)
            tryVerify(function() { return findChild(view, "historyEntry-0") !== null })
            const entry = findChild(view, "historyEntry-0")
            compare(findChild(view, "historyEdition-0").text, "Unknown edition")
            compare(entry.enabled, false)
            const previousTitle = ArticleState.currentPageTitle
            mouseClick(entry)
            compare(ArticleState.currentPageTitle, previousTitle)
            compare(testSupport.requestCount, 0)
            compare(HistoryState.history[0].language, "en")
        }

        function test_article_visits_are_ordered_and_deduplicated() {
            loadArticle(firstArticleId)
            compare(HistoryState.history.length, 1)
            compare(HistoryState.history[0].pageId, firstArticleId)
            compare(HistoryState.history[0].title, "First history article")

            loadArticle(secondArticleId)
            compare(HistoryState.history.length, 2)
            compare(HistoryState.history[0].pageId, secondArticleId)
            compare(HistoryState.history[1].pageId, firstArticleId)

            // The second visit is served from ArticleState's article cache. It
            // still records a visit, moving the existing entry to the front.
            loadArticle(firstArticleId)
            compare(HistoryState.history.length, 2)
            compare(HistoryState.history[0].pageId, firstArticleId)
            compare(HistoryState.history[1].pageId, secondArticleId)
            compare(testSupport.requestCount, 4)
        }
    }
}
