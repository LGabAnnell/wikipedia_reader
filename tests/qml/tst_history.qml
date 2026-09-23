import QtQuick
import QtTest
import wikipedia_qt

Item {
    id: root
    width: 800
    height: 600

    TestCase {
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
            GlobalState.loadArticleByPageId(pageId)
            tryCompare(GlobalState, "currentPageId", pageId)
            tryCompare(GlobalState, "isLoading", false)
        }

        function init() {
            testSupport.clearRequests()
            testSupport.clearQmlWarnings()
            HistoryState.clearHistory()
            GlobalState.currentImageUrl = ""
            GlobalState.currentImageDescription = ""
            configureArticleFixture(firstArticleId, "First history article")
            configureArticleFixture(secondArticleId, "Second history article")
        }

        function cleanup() {
            while (networkFixtures.pendingReplyCount > 0)
                networkFixtures.completeNextReply()
            GlobalState.currentImageUrl = ""
            GlobalState.currentImageDescription = ""
            HistoryState.clearHistory()
            compare(testSupport.qmlWarningCount, 0, testSupport.qmlWarnings.join("\n"))
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

            // The second visit is served from GlobalState's article cache. It
            // still records a visit, moving the existing entry to the front.
            loadArticle(firstArticleId)
            compare(HistoryState.history.length, 2)
            compare(HistoryState.history[0].pageId, firstArticleId)
            compare(HistoryState.history[1].pageId, secondArticleId)
            compare(testSupport.requestCount, 4)
        }
    }
}
