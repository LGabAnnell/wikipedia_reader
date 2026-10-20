import QtQuick
import QtQuick.Controls
import QtTest
import wikipedia_qt
import wikipedia_qt.ContentDisplay
import wikipedia_qt.ImageDisplay

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
            articleText: ArticleState.currentPageExtract
        }
    }

    Component {
        id: imageViewComponent
        ImageView {
            onBackRequested: stackView.pop()
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
        NavigationState.addView(Constants.imageView, imageViewComponent)
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

        function resolveUrl(title) {
            return "https://en.wikipedia.org/w/api.php?action=query&format=json"
                    + "&titles=" + encodeURIComponent(title) + "&prop=pageprops%7Cpageids"
        }

        function addArticleFixtures(pageId, title, htmlText) {
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
                "text": htmlText
            }}))
            networkFixtures.addFixture("GET", sectionsUrl(title), JSON.stringify({"parse": {
                "tocdata": {"sections": []}
            }}))
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
                "text": "<p>Rendered article fixture body</p><figure><a href='/wiki/File:Borden.jpg'><img alt='Old image alt' src='//upload.wikimedia.org/wikipedia/commons/fixture.png' width='250'></a><figcaption>A Borden house &amp; garden</figcaption></figure>"
            }}))
            networkFixtures.addFixture("GET", sectionsUrl(title), JSON.stringify({"parse": {
                "tocdata": {"sections": []}
            }}))
        }

        function resetState() {
            while (stackView.depth > 1)
                stackView.pop()
            wait(250)
            ArticleState.setCurrentPageFromData("", "", "")
            ArticleState.clearErrorMessage()
            ArticleState.setIsLoading(false)
            SearchState.reset()
            ImageSelectionState.selectImage("", "")
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
            tryCompare(ArticleState, "currentPageId", successArticleId)
            tryCompare(ArticleState, "currentPageTitle", successTitle)
            tryCompare(ArticleState, "isLoading", false)
            tryCompare(networkFixtures, "requestCount", 5)
            tryCompare(networkFixtures, "pendingReplyCount", 0)

            const title = child("articleTitle")
            tryCompare(title, "text", successTitle)
            const body = child("articleBody")
            tryVerify(function() { return body.text.indexOf("Rendered article fixture body") >= 0 })
            const imageLink = body.text.match(/href="(wikipedia-image:[^"]+)"/)
            verify(imageLink !== null, "Processed figure must render as an image-view link")
            const imageData = JSON.parse(decodeURIComponent(
                imageLink[1].substring("wikipedia-image:".length)))
            compare(imageData.url,
                    "https://upload.wikimedia.org/wikipedia/commons/fixture.png")
            compare(imageData.description, "A Borden house & garden")
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

        function test_inline_image_selection_and_back_navigation() {
            const inlineArticleId = 803
            const inlineTitle = "Inline image fixture"
            addSuccessArticleFixtures(inlineArticleId, inlineTitle)
            ArticleState.loadArticleByPageId(inlineArticleId)
            tryCompare(ArticleState, "isLoading", false)
            NavigationState.navigateToContent()
            tryCompare(stackView, "depth", 2)
            tryVerify(function() { return !stackView.busy })
            const articleView = stackView.currentItem
            const body = child("articleBody")
            const imageLink = body.text.match(/href="(wikipedia-image:[^"]+)"/)
            verify(imageLink !== null)
            body.linkActivated(imageLink[1])
            tryCompare(stackView, "depth", 3)
            tryVerify(function() { return !stackView.busy })
            compare(ImageSelectionState.currentImageUrl,
                    "https://upload.wikimedia.org/wikipedia/commons/fixture.png")
            compare(ImageSelectionState.currentImageDescription, "A Borden house & garden")
            tryCompare(child("fullScreenImage"), "status", Image.Ready)
            compare(child("imageCaptionText").text, "A Borden house & garden")
            mouseClick(child("imageBackButton"))
            tryCompare(stackView, "depth", 2)
            tryVerify(function() { return !stackView.busy })
            compare(stackView.currentItem, articleView)
            compare(ArticleState.currentPageTitle, inlineTitle)
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

            tryCompare(ArticleState, "isLoading", false)
            tryCompare(ArticleState, "errorMessage", "HTTP 500")
            const error = child("articleErrorMessage")
            tryCompare(error, "visible", true)
            compare(error.text, "HTTP 500")
            compare(child("articleLoadingIndicator").visible, false)
            compare(HistoryState.history.length, 0)
            compare(testSupport.requestCount, 2)
        }

        function test_language_change_clears_covered_article_views_without_navigation() {
            const firstTitle = "Language transition first"
            const secondTitle = "Language transition second"
            networkFixtures.addFixture("GET", sectionsUrl(firstTitle),
                                       '{"parse":{"tocdata":{"sections":[]}}}')
            networkFixtures.addFixture("GET", sectionsUrl(secondTitle),
                                       '{"parse":{"tocdata":{"sections":[]}}}')

            ArticleState.setCurrentPageFromData(firstTitle, "<p>Old edition first</p>", "")
            NavigationState.navigateToContent()
            tryCompare(stackView, "depth", 2)
            tryVerify(function() { return !stackView.busy })
            const firstView = stackView.currentItem
            tryVerify(function() { return findChild(firstView, "articleBody").text.indexOf("Old edition first") >= 0 })

            ArticleState.setCurrentPageFromData(secondTitle, "<p>Old edition second</p>", "")
            NavigationState.navigateToContent()
            tryCompare(stackView, "depth", 3)
            tryVerify(function() { return !stackView.busy })
            const secondView = stackView.currentItem
            tryVerify(function() { return findChild(secondView, "articleBody").text.indexOf("Old edition second") >= 0 })

            SettingsState.setLanguage("fr")
            compare(stackView.depth, 3)
            compare(stackView.currentItem, secondView)
            compare(ArticleState.currentPageTitle, "")
            compare(findChild(secondView, "articleTitle").text, "")
            verify(findChild(secondView, "articleBody").text.indexOf("Old edition second") < 0)
            stackView.pop()
            tryCompare(stackView, "depth", 2)
            compare(stackView.currentItem, firstView)
            compare(findChild(firstView, "articleTitle").text, "")
            verify(findChild(firstView, "articleBody").text.indexOf("Old edition first") < 0)
            SettingsState.setLanguage("en")
        }

        function test_link_click_pushes_article_and_back_restores_previous_article_data() {
            return [
                {tag: "plain", firstId: 811, title: "Second article fixture", link: "/wiki/Second_article_fixture"},
                {tag: "unicode", firstId: 813, title: "São Paulo", link: "/wiki/S%C3%A3o_Paulo"},
                {tag: "percent", firstId: 815, title: "100% coverage", link: "/wiki/100%25_coverage"},
                {tag: "encoded_underscore", firstId: 817, title: "Encoded space", link: "/wiki/Encoded%5Fspace"}
            ]
        }

        function test_link_click_pushes_article_and_back_restores_previous_article(data) {
            const firstId = data.firstId
            const firstTitle = "First article fixture"
            const secondId = firstId + 1
            const secondTitle = data.title
            const secondLink = data.link

            addArticleFixtures(firstId, firstTitle,
                               "<p>First article body.</p>"
                               + "<p>See <a href=\"" + secondLink + "\">the second article</a>.</p>")
            const resolvePages = {}
            resolvePages[secondId] = {"pageid": secondId, "title": secondTitle}
            networkFixtures.addFixture("GET", resolveUrl(secondTitle),
                                       JSON.stringify({"query": {"pages": resolvePages}}))
            addArticleFixtures(secondId, secondTitle, "<p>Second article body.</p>")

            // Show the first article.
            ArticleState.loadArticleByPageId(firstId)
            tryCompare(ArticleState, "isLoading", false)
            tryCompare(ArticleState, "currentPageTitle", firstTitle)
            NavigationState.navigateToContent()
            tryCompare(stackView, "depth", 2)
            tryVerify(function() { return !stackView.busy })
            const firstView = stackView.currentItem
            const firstBody = findChild(firstView, "articleBody")
            verify(firstBody !== null, "First article view must have a body")
            tryVerify(function() { return firstBody.text.indexOf("First article body") >= 0 })
            compare(findChild(firstView, "articleTitle").text, firstTitle)

            // Clicking an article link pushes a new article view onto the stack.
            const linkMatch = firstBody.text.match(/href="(\/wiki\/[^"]+)"/)
            verify(linkMatch !== null, "Rendered article must contain a /wiki/ link")
            networkFixtures.deferNextReply()
            firstBody.linkActivated(linkMatch[1])
            tryCompare(stackView, "depth", 3)
            tryVerify(function() { return !stackView.busy })
            const secondView = stackView.currentItem
            verify(secondView !== firstView, "Link click must push a new article view")
            compare(ArticleState.isLoading, true)
            compare(findChild(secondView, "articleLoadingIndicator").visible, true)
            compare(findChild(secondView, "articleTitle").text, "",
                    "New article view must not flash the previous article")

            // The linked article loads into the new view.
            networkFixtures.completeNextReply()
            tryCompare(ArticleState, "currentPageTitle", secondTitle)
            tryCompare(ArticleState, "isLoading", false)
            const secondBody = findChild(secondView, "articleBody")
            tryVerify(function() { return secondBody.text.indexOf("Second article body") >= 0 })
            compare(findChild(secondView, "articleTitle").text, secondTitle)

            // Page query + parse + sections per article, plus the title resolve.
            tryCompare(networkFixtures, "requestCount", 7)
            compare(HistoryState.history.length, 2)
            compare(HistoryState.history[0].pageId, secondId)

            // Back pops to the first article, which kept its own content even
            // though the global state now points at the second article.
            stackView.pop()
            tryCompare(stackView, "depth", 2)
            tryVerify(function() { return !stackView.busy })
            compare(stackView.currentItem, firstView)
            compare(findChild(firstView, "articleTitle").text, firstTitle)
            tryVerify(function() {
                return findChild(firstView, "articleBody").text.indexOf("First article body") >= 0
            })
            compare(ArticleState.currentPageTitle, secondTitle)

            // The covered view did not refetch its sections.
            wait(100)
            compare(networkFixtures.requestCount, 7)
        }

        function test_malformed_article_link_preserves_current_article_data() {
            return [
                {tag: "incomplete_escape", pageId: 831, link: "/wiki/Bad%"},
                {tag: "invalid_escape", pageId: 832, link: "/wiki/Bad%GG"},
                {tag: "invalid_utf8", pageId: 833, link: "/wiki/Bad%C3%28"}
            ]
        }

        function test_malformed_article_link_preserves_current_article(data) {
            const title = "Malformed link fixture " + data.tag
            addArticleFixtures(data.pageId, title, "<p>Current article body.</p>")
            ArticleState.loadArticleByPageId(data.pageId)
            tryCompare(ArticleState, "isLoading", false)
            tryCompare(ArticleState, "currentPageTitle", title)
            NavigationState.navigateToContent()
            tryCompare(stackView, "depth", 2)
            tryVerify(function() { return !stackView.busy })
            tryCompare(networkFixtures, "requestCount", 3)
            const view = stackView.currentItem
            const body = findChild(view, "articleBody")
            tryVerify(function() { return body.text.indexOf("Current article body") >= 0 })
            const previousText = body.text
            const previousExtract = ArticleState.currentPageExtract

            ignoreWarning(/Unable to decode article link.*/)
            body.linkActivated(data.link)
            wait(100)

            compare(networkFixtures.requestCount, 3)
            compare(stackView.depth, 2)
            compare(stackView.currentItem, view)
            compare(ArticleState.currentPageTitle, title)
            compare(ArticleState.currentPageExtract, previousExtract)
            compare(ArticleState.isLoading, false)
            compare(ArticleState.errorMessage, "")
            compare(body.text, previousText)
            compare(HistoryState.history.length, 1)

            // A rejected link must not freeze this view's article bindings.
            ArticleState.setCurrentPageFromData(title, "<p>Updated article body.</p>", "")
            tryVerify(function() { return body.text.indexOf("Updated article body") >= 0 })
        }
    }
}
