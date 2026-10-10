import QtQuick
import QtTest
import wikipedia_qt
import wikipedia_qt.SearchBar
import wikipedia_qt.Section
import wikipedia_qt.ImageDisplay

Item {
    id: root

    Component { id: searchComponent; SearchBarModel {} }
    Component { id: sectionComponent; SectionModel {} }
    Component { id: galleryComponent; ImageHomeModel {} }

    TestCase {
        name: "FeatureIsolation"
        property var model: null
        readonly property string articleUrl: "https://en.wikipedia.org/w/api.php?action=query&format=json&prop=extracts%7Cimages&pageids=901&explaintext=1&imlimit=50"
        readonly property string searchUrl: "https://en.wikipedia.org/w/api.php?action=query&format=json&list=search&srsearch=isolation&srlimit=10"
        readonly property string sectionUrl: "https://en.wikipedia.org/w/api.php?action=parse&format=json&prop=tocdata&page=Isolation"
        readonly property string galleryUrl: "https://en.wikipedia.org/w/api.php?action=query&format=json&prop=extracts%7Cimages&pageids=901&explaintext=1&imlimit=50"

        function init() {
            testSupport.clearRequests()
            testSupport.clearQmlWarnings()
            SearchState.reset()
            ArticleState.setCurrentPageFromData("Original article", "<p>Original HTML</p>", "")
            ArticleState.clearErrorMessage()
            HistoryState.clearHistory()
            networkFixtures.addFixture("GET", articleUrl, "article failure", "text/plain", 500)
        }

        function cleanup() {
            while (networkFixtures.pendingReplyCount > 0)
                networkFixtures.completeNextReply()
            tryCompare(ArticleState, "isLoading", false)
            tryCompare(SearchState, "isSearching", false)
            if (model)
                model.destroy()
            model = null
            SearchState.reset()
            ArticleState.setCurrentPageFromData("", "", "")
            ArticleState.clearErrorMessage()
            compare(testSupport.qmlWarningCount, 0, testSupport.qmlWarnings.join("\n"))
        }

        function beginArticleRequest() {
            networkFixtures.deferNextReply()
            ArticleState.loadArticleByPageId(901)
            ArticleState.setErrorMessage("article feedback")
            compare(ArticleState.isLoading, true)
            compare(networkFixtures.pendingReplyCount, 2)
        }

        function verifyArticleUnchanged() {
            compare(ArticleState.isLoading, true)
            compare(ArticleState.errorMessage, "article feedback")
            compare(ArticleState.currentPageTitle, "Original article")
            compare(ArticleState.currentPageExtract, "<p>Original HTML</p>")
            compare(HistoryState.history.length, 0)
            compare(networkFixtures.pendingReplyCount, 1)
        }

        function test_search_response_does_not_complete_article_data() {
            return [
                { tag: "populated", status: 200, body: '{"query":{"search":[{"title":"Result","snippet":"Snippet","pageid":42}]}}', count: 1 },
                { tag: "empty", status: 200, body: '{"query":{"search":[]}}', count: 0 },
                { tag: "failure", status: 500, body: 'search failure', count: 0 }
            ]
        }

        function test_search_response_does_not_complete_article(data) {
            networkFixtures.addFixture("GET", searchUrl, data.body, "application/json", data.status)
            model = searchComponent.createObject(root)
            model.searchText = "isolation"
            networkFixtures.deferNextReply()
            model.performSearch()
            compare(SearchState.isSearching, true)
            beginArticleRequest()
            networkFixtures.completeNextReply()
            tryCompare(SearchState, "isSearching", false)
            compare(SearchState.hasCompletedSearch, data.status === 200)
            compare(SearchState.searchResults.length, data.count)
            compare(SearchState.errorMessage, data.status === 200 ? "" : "HTTP 500")
            verifyArticleUnchanged()
        }

        function test_search_does_not_clear_existing_article_error() {
            ArticleState.setErrorMessage("previous article failure")
            networkFixtures.addFixture("GET", searchUrl, '{"query":{"search":[]}}')
            networkFixtures.deferNextReply()
            SearchState.search("isolation")
            compare(ArticleState.isLoading, false)
            compare(ArticleState.errorMessage, "previous article failure")
            networkFixtures.completeNextReply()
            tryCompare(SearchState, "isSearching", false)
            compare(ArticleState.isLoading, false)
            compare(ArticleState.errorMessage, "previous article failure")
        }

        function test_section_failure_does_not_complete_article() {
            networkFixtures.addFixture("GET", sectionUrl, "section failure", "text/plain", 500)
            model = sectionComponent.createObject(root)
            networkFixtures.deferNextReply()
            model.fetchSections("Isolation")
            compare(model.isLoading, true)
            beginArticleRequest()
            networkFixtures.completeNextReply()
            tryCompare(model, "isLoading", false)
            compare(model.errorMessage, "HTTP 500")
            verifyArticleUnchanged()
        }

        function test_gallery_failure_does_not_complete_article() {
            networkFixtures.addFixture("GET", galleryUrl, "gallery failure", "text/plain", 500)
            model = galleryComponent.createObject(root)
            networkFixtures.deferNextReply()
            model.loadImagesForPage(901)
            compare(model.isLoading, true)
            beginArticleRequest()
            networkFixtures.completeNextReply()
            tryCompare(model, "isLoading", false)
            compare(model.errorMessage, "HTTP 500")
            verifyArticleUnchanged()
        }
    }
}
