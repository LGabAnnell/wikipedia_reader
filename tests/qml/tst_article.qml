import QtQuick
import QtQuick.Controls
import QtTest
import wikipedia_qt
import wikipedia_qt.ContentDisplay

Item {
    id: root
    width: 800
    height: 600

    Component {
        id: searchScreenComponent

        SearchScreen {}
    }

    Component {
        id: articleComponent

        ContentDisplay {
            articleText: GlobalState.currentPageExtract
        }
    }

    StackView {
        id: stackView
        objectName: "articleTestStack"
        anchors.fill: parent
        initialItem: searchScreenComponent
    }

    Component.onCompleted: {
        NavigationState.setStackView(stackView)
        NavigationState.addView(Constants.contentView, articleComponent)
    }

    Connections {
        target: NavigationState
        function onPushView(view) {
            stackView.push(view)
        }
    }

    TestCase {
        name: "ArticleBehavior"
        when: windowShown

        readonly property int successArticleId: 801
        readonly property int failureArticleId: 802
        readonly property string successTitle: "Article flow fixture"
        readonly property string failureTitle: "Failed article fixture"

        function searchUrl(query) {
            return "https://en.wikipedia.org/w/api.php?action=query&format=json&list=search"
                    + "&srsearch=" + query + "&srlimit=10"
        }

        function pageQueryUrl(pageId) {
            return "https://en.wikipedia.org/w/api.php?action=query&format=json"
                    + "&prop=extracts%7Cimages&pageids=" + pageId
                    + "&explaintext=1&imlimit=50"
        }

        function parseUrl(pageId) {
            return "https://en.wikipedia.org/w/api.php?action=parse&disableeditsection=true"
                    + "&format=json&formatversion=2&pageid=" + pageId + "&prop=text"
        }

        function sectionsUrl(title) {
            return "https://en.wikipedia.org/w/api.php?action=parse&format=json"
                    + "&prop=tocdata&page=" + encodeURIComponent(title)
        }

        function addSearchResult(query, title, pageId) {
            networkFixtures.addFixture("GET", searchUrl(query), JSON.stringify({
                "query": {"search": [{"title": title, "snippet": "Article fixture result", "pageid": pageId}]}
            }))
        }

        function addSuccessArticleFixtures(pageId, title) {
            const pages = {}
            pages[pageId] = {
                "pageid": pageId,
                "title": title,
                "extract": title + " extract",
                "images": []
            }
            networkFixtures.addFixture("GET", pageQueryUrl(pageId), JSON.stringify({"query": {"pages": pages}}))
            networkFixtures.addFixture("GET", parseUrl(pageId), JSON.stringify({"parse": {
                "title": title,
                "pageid": pageId,
                "text": "<p>Rendered article fixture body</p>"
            }}))
            networkFixtures.addFixture("GET", sectionsUrl(title), JSON.stringify({"parse": {
                "tocdata": {"sections": []}
            }}))
        }

        function resetState() {
            while (stackView.depth > 1)
                stackView.pop()
            wait(250)
            GlobalState.setCurrentPageFromData("", "", "")
            GlobalState.clearErrorMessage()
            GlobalState.setIsLoading(false)
            GlobalState.setSections([])
            GlobalState.setLoadingSections(false)
            GlobalState.setSearchResults([])
            HistoryState.clearHistory()
            const input = findChild(root, "searchInput")
            if (input)
                input.text = ""
        }

        function init() {
            resetState()
            testSupport.clearRequests()
            testSupport.clearQmlWarnings()
        }

        function cleanup() {
            while (networkFixtures.pendingReplyCount > 0)
                networkFixtures.completeNextReply()
            resetState()
            compare(testSupport.qmlWarningCount, 0, testSupport.qmlWarnings.join("\n"))
        }

        function child(name) {
            const item = findChild(root, name)
            verify(item !== null, "Missing object named " + name)
            return item
        }

        function runSearch(query) {
            const input = child("searchInput")
            const button = child("searchButton")
            input.text = query
            verify(button.enabled)
            mouseClick(button)
            tryCompare(networkFixtures, "requestCount", 1)
            tryCompare(child("searchResultsList"), "count", 1)
        }

        function test_selecting_result_loads_and_displays_article() {
            addSearchResult("article", successTitle, successArticleId)
            addSuccessArticleFixtures(successArticleId, successTitle)
            runSearch("article")

            networkFixtures.deferNextReply()
            mouseClick(child("searchResultContent-0"))
            tryCompare(stackView, "depth", 2)
            tryCompare(networkFixtures, "pendingReplyCount", 1)
            compare(child("articleLoadingIndicator").visible, true)
            compare(testSupport.requestCount, 2)

            networkFixtures.completeNextReply()
            tryCompare(GlobalState, "currentPageId", successArticleId)
            tryCompare(GlobalState, "currentPageTitle", successTitle)
            tryCompare(GlobalState, "isLoading", false)
            tryCompare(networkFixtures, "requestCount", 4)
            tryCompare(networkFixtures, "pendingReplyCount", 0)

            const title = child("articleTitle")
            tryCompare(title, "text", successTitle)
            const body = child("articleBody")
            tryVerify(function() { return body.text.indexOf("Rendered article fixture body") >= 0 })
            compare(child("articleLoadingIndicator").visible, false)
            compare(child("articleErrorMessage").visible, false)
            compare(HistoryState.history.length, 1)
            compare(HistoryState.history[0].pageId, successArticleId)

            const requests = testSupport.requests
            verify(requests.some(function(request) {
                return request.query.indexOf("prop=text") >= 0
            }), "The article HTML parse request must be made")
            verify(requests.some(function(request) {
                return request.query.indexOf("prop=tocdata") >= 0
            }), "The article sections request must be made")
        }

        function test_article_load_error_is_shown_without_history_visit() {
            addSearchResult("article-error", failureTitle, failureArticleId)
            networkFixtures.addFixture("GET", pageQueryUrl(failureArticleId), "failure", "text/plain", 500)
            runSearch("article-error")

            networkFixtures.deferNextReply()
            mouseClick(child("searchResultContent-0"))
            tryCompare(stackView, "depth", 2)
            tryCompare(networkFixtures, "pendingReplyCount", 1)
            compare(child("articleLoadingIndicator").visible, true)
            networkFixtures.completeNextReply()

            tryCompare(GlobalState, "isLoading", false)
            tryCompare(GlobalState, "errorMessage", "HTTP 500")
            const error = child("articleErrorMessage")
            tryCompare(error, "visible", true)
            compare(error.text, "HTTP 500")
            compare(child("articleLoadingIndicator").visible, false)
            compare(HistoryState.history.length, 0)
            compare(testSupport.requestCount, 2)
        }
    }
}
