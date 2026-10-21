import QtQuick
import QtTest
import wikipedia_qt
import wikipedia_qt.Home

Item {
    id: root
    width: 800
    height: 600

    Component {
        id: mainComponent
        Main {}
    }

    TestCase {
        name: "ApplicationNavigation"
        when: windowShown

        property var appWindow: null
        property var appStack: null
        property var homeRoot: null

        function registerHomeFixtures() {
            const date = Qt.formatDate(new Date(), "yyyy/MM/dd")
            const monthDay = Qt.formatDate(new Date(), "MM/dd")
            for (const lang of ["en", "fr"]) {
                const featuredUrl = "https://" + lang + ".wikipedia.org/api/rest_v1/feed/featured/" + date
                const onThisDayUrl = "https://api.wikimedia.org/feed/v1/wikipedia/" + lang + "/onthisday/all/" + monthDay
                const randomTitleUrl = "https://" + lang + ".wikipedia.org/api/rest_v1/page/random/title"
                const title = lang === "en" ? "Navigation fixture" : "Navigation fixture français"
                const article = {"title":title,"extract":"Article summary","pageid":1,"content_urls":{"desktop":{"page":"https://" + lang + ".wikipedia.org/wiki/Fixture"}}}
                const feed = {
                    "tfa":{"title":title,"extract":"Fixture article","pageid":1},
                    "news":[{"story":"A news story", "links":[article]}]
                }
                networkFixtures.addFixture("GET", featuredUrl, JSON.stringify(feed))
                const popularArticles = [article]
                for (let pageId = 2; pageId <= 43; ++pageId)
                    popularArticles.push({"title":"Article " + pageId,"extract":"Summary " + pageId,"pageid":pageId})
                for (let daysBack = 1; daysBack <= 2; ++daysBack) {
                    const previousDate = new Date()
                    previousDate.setDate(previousDate.getDate() - daysBack)
                    const previousFeedUrl = "https://" + lang + ".wikipedia.org/api/rest_v1/feed/featured/" + Qt.formatDate(previousDate, "yyyy/MM/dd")
                    const previousFeed = lang === "en"
                        ? {"mostread":{"articles":popularArticles}}
                        : {}
                    networkFixtures.addFixture("GET", previousFeedUrl, JSON.stringify(previousFeed))
                }
                networkFixtures.addFixture("GET", onThisDayUrl, '{"selected":[]}')
                networkFixtures.addFixture("GET", randomTitleUrl, '{"items":[]}')
            }
        }

        function init() {
            testSupport.clearRequests()
            testSupport.clearQmlWarnings()
            if (!appWindow) {
                registerHomeFixtures()
                appWindow = mainComponent.createObject(root)
                verify(appWindow !== null, "Main application window must be created")
                appStack = findChild(appWindow, "applicationStack")
                verify(appStack !== null, "Main StackView must be available")
                tryCompare(testSupport, "requestCount", 5)
                homeRoot = appStack.currentItem
                verify(homeRoot !== null, "Home must be the initial stack item")
                compare(homeRoot.objectName, "homeView")
            } else {
                const homeButton = findChild(appWindow, "homeButton")
                verify(homeButton !== null)
                mouseClick(homeButton)
                tryCompare(appStack, "depth", 1)
            }
            compare(appStack.depth, 1)
            compare(appStack.currentItem, homeRoot)
        }

        function cleanup() {
            if (SettingsState.language !== "en") {
                SettingsState.setLanguage("en")
                tryCompare(child("homeModel").newsItems, "length", 43)
            }
            HistoryState.clearHistory()
            compare(testSupport.qmlWarningCount, 0, testSupport.qmlWarnings.join("\n"))
        }

        function child(name) {
            const item = findChild(appWindow, name)
            verify(item !== null, "Missing object named " + name)
            return item
        }


        function historyChild(name, index) {
            const list = findChild(appStack.currentItem, "historyList")
            verify(list !== null)
            tryVerify(function() { return list.itemAtIndex(index) !== null })
            const item = findChild(list.itemAtIndex(index), name)
            verify(item !== null, "Missing history object " + name)
            return item
        }

        function configureHistoryArticle(language, pageId, title) {
            const prefix = "https://" + language + ".wikipedia.org/w/api.php"
            const pages = {}
            pages[pageId] = {pageid:pageId, title:title, extract:"", images:[]}
            networkFixtures.addFixture("GET", prefix + "?action=query&format=json&pageids=" + pageId,
                                       JSON.stringify({query:{pages:pages}}))
            networkFixtures.addFixture("GET", prefix + "?action=parse&format=json&prop=tocdata&page=" + encodeURIComponent(title),
                                       '{"parse":{"tocdata":{"sections":[]}}}')
            networkFixtures.addFixture("GET", prefix + "?action=parse&disableeditsection=true&format=json&formatversion=2&pageid=" + pageId + "&prop=text",
                                       JSON.stringify({parse:{text:"<p>" + title + " body</p>"}}))
        }

        function test_history_restores_edition_renders_article_and_back_returns_to_history() {
            configureHistoryArticle("fr", 701, "French history destination")
            HistoryState.addToHistory("English identity", 701, "en")
            HistoryState.addToHistory("French history destination", 701, "fr")
            mouseClick(child("historyButton"))
            tryCompare(appStack, "depth", 2)
            tryVerify(function() { return !appStack.busy })
            const historyView = appStack.currentItem
            compare(historyChild("historyEdition-0", 0).text, "FR Wikipedia")
            compare(historyChild("historyEdition-1", 1).text, "EN Wikipedia")
            testSupport.clearRequests()
            mouseClick(historyChild("historyEntry-0", 0))
            tryCompare(appStack, "depth", 3)
            tryVerify(function() { return !appStack.busy })
            tryCompare(ArticleState, "isLoading", false)
            compare(SettingsState.language, "fr")
            compare(child("languageSelector").currentValue, "fr")
            compare(child("articleTitle").text, "French history destination")
            verify(child("articleBody").text.indexOf("French history destination body") >= 0)
            verify(testSupport.requests.some(function(request) {
                return request.host === "fr.wikipedia.org" && request.url.indexOf("pageids=701") >= 0
            }))
            verify(!testSupport.requests.some(function(request) {
                return request.host === "en.wikipedia.org" && request.url.indexOf("701") >= 0
            }))
            compare(HistoryState.history.length, 2)
            mouseClick(child("backButton"))
            tryCompare(appStack, "depth", 2)
            tryVerify(function() { return !appStack.busy })
            compare(appStack.currentItem, historyView)
            mouseClick(child("clearHistoryButton"))
            compare(HistoryState.history.length, 0)
            compare(child("historyList").count, 0)
            compare(child("clearHistoryButton").enabled, false)
        }

        function test_history_cached_click_copies_identity_before_reordering() {
            configureHistoryArticle("en", 702, "Cached first destination")
            configureHistoryArticle("en", 703, "Cached second destination")
            ArticleState.loadArticleByPageId(702)
            tryCompare(ArticleState, "isLoading", false)
            ArticleState.loadArticleByPageId(703)
            tryCompare(ArticleState, "isLoading", false)
            compare(HistoryState.history[1].pageId, 702)
            mouseClick(child("historyButton"))
            tryCompare(appStack, "depth", 2)
            tryVerify(function() { return !appStack.busy })
            testSupport.clearRequests()
            mouseClick(historyChild("historyEntry-1", 1))
            tryCompare(appStack, "depth", 3)
            tryVerify(function() { return !appStack.busy })
            compare(ArticleState.currentPageId, 702)
            compare(child("articleTitle").text, "Cached first destination")
            compare(HistoryState.history[0].pageId, 702)
            // The content view requests sections; the article itself is cached.
            compare(testSupport.requestCount, 1)
            verify(testSupport.requests[0].query.indexOf("prop=tocdata") >= 0)
        }

        function test_header_search_and_history_push_and_back_pops_one_view() {
            mouseClick(child("searchButton"))
            tryCompare(appStack, "depth", 2)
            tryVerify(function() { return !appStack.busy })
            compare(appStack.currentItem.objectName, "searchView")

            mouseClick(child("historyButton"))
            tryCompare(appStack, "depth", 3)
            tryVerify(function() { return !appStack.busy })
            compare(appStack.currentItem.objectName, "historyView")

            mouseClick(child("backButton"))
            tryCompare(appStack, "depth", 2)
            tryVerify(function() { return !appStack.busy })
            compare(appStack.currentItem.objectName, "searchView")

            mouseClick(child("backButton"))
            tryCompare(appStack, "depth", 1)
            tryVerify(function() { return !appStack.busy })
            compare(appStack.currentItem, homeRoot)
            verify(testSupport.sendBackButtonPress(appWindow))
            compare(appStack.depth, 1)
            compare(appStack.currentItem, homeRoot)
        }

        function test_invalid_view_name_data() {
            return [
                { tag: "empty", viewName: "" },
                { tag: "unknown", viewName: "unregisteredView" }
            ]
        }

        function test_invalid_view_name(data) {
            const originalDepth = appStack.depth
            const originalItem = appStack.currentItem
            ignoreWarning('Cannot navigate to unregistered view: "' + data.viewName + '"')
            NavigationState.navigateToView(data.viewName)
            compare(appStack.depth, originalDepth)
            compare(appStack.currentItem, originalItem)

            NavigationState.navigateToView(Constants.searchView)
            tryCompare(appStack, "depth", originalDepth + 1)
            tryVerify(function() { return !appStack.busy })
            compare(appStack.currentItem.objectName, "searchView")
        }

        function test_history_navigation_does_not_duplicate_the_current_view() {
            mouseClick(child("historyButton"))
            tryCompare(appStack, "depth", 2)
            const firstHistoryView = appStack.currentItem
            // Cover direct NavigationState callers and rapid header presses
            // before the initial push transition has finished.
            NavigationState.navigateToHistory()
            mouseClick(child("historyButton"))
            tryCompare(appStack, "depth", 2)
            tryVerify(function() { return !appStack.busy })
            compare(appStack.currentItem, firstHistoryView)
            compare(appStack.currentItem.objectName, "historyView")

            // An older history view lower in the stack must not prevent a
            // fresh history view from opening over another screen.
            mouseClick(child("searchButton"))
            tryCompare(appStack, "depth", 3)
            tryVerify(function() { return !appStack.busy })
            mouseClick(child("historyButton"))
            tryCompare(appStack, "depth", 4)
            tryVerify(function() { return !appStack.busy })
            compare(appStack.currentItem.objectName, "historyView")

            NavigationState.navigateToHistory()
            mouseClick(child("historyButton"))
            compare(appStack.depth, 4)

            // Back restores the earlier history view; pressing History then
            // remains idempotent because that view is now on top.
            mouseClick(child("backButton"))
            tryCompare(appStack, "depth", 3)
            tryVerify(function() { return !appStack.busy })
            mouseClick(child("backButton"))
            tryCompare(appStack, "depth", 2)
            tryVerify(function() { return !appStack.busy })
            compare(appStack.currentItem.objectName, "historyView")
            mouseClick(child("historyButton"))
            compare(appStack.depth, 2)
        }

        function test_home_returns_to_the_original_root_and_is_idempotent() {
            mouseClick(child("searchButton"))
            tryCompare(appStack, "depth", 2)
            tryVerify(function() { return !appStack.busy })
            mouseClick(child("historyButton"))
            tryCompare(appStack, "depth", 3)
            tryVerify(function() { return !appStack.busy })

            mouseClick(child("homeButton"))
            tryCompare(appStack, "depth", 1)
            compare(appStack.currentItem, homeRoot)

            mouseClick(child("homeButton"))
            compare(appStack.depth, 1)
            compare(appStack.currentItem, homeRoot)
        }

        function test_home_uses_recent_mostread_when_today_only_has_news() {
            const homeModel = child("homeModel")
            tryCompare(homeModel.newsItems, "length", 43)
            compare(homeModel.newsItems[0].title, "Navigation fixture")
            compare(homeModel.newsItems[0].description, "Article summary")
            compare(homeModel.newsItems[42].title, "Article 43")
            compare(homeModel.newsItems[42].pageid, 43)
            const newsItems = child("homeNewsItems")
            tryCompare(newsItems, "count", 43)
            verify(newsItems.itemAt(42) !== null)
            tryVerify(function() { return newsItems.itemAt(42).height > 0 })
            const scrollView = child("homeScrollView")
            tryVerify(function() { return scrollView.contentHeight > scrollView.availableHeight })
        }

        function test_featured_article_navigation_and_back() {
            configureHistoryArticle("en", 1, "Navigation fixture")
            const homeModel = child("homeModel")
            tryCompare(homeModel, "featuredStatus", HomeModel.Ready)
            mouseClick(child("featuredTitle"), 5, child("featuredTitle").height / 2)
            tryCompare(appStack, "depth", 2)
            tryVerify(function() { return !appStack.busy })
            tryCompare(ArticleState, "isLoading", false)
            compare(ArticleState.currentPageId, 1)
            mouseClick(child("backButton"))
            tryCompare(appStack, "depth", 1)
            compare(appStack.currentItem, homeRoot)
        }

        function test_mouse_back_button_pops_one_view() {
            mouseClick(child("searchButton"))
            tryCompare(appStack, "depth", 2)
            tryVerify(function() { return !appStack.busy })

            verify(testSupport.sendBackButtonPress(appWindow), "Back button event should be handled")
            tryCompare(appStack, "depth", 1)
            compare(appStack.currentItem, homeRoot)
        }

        function test_language_change_keeps_home_and_refreshes_its_content() {
            const homeModel = child("homeModel")
            tryCompare(homeModel.newsItems, "length", 43)
            testSupport.clearRequests()
            SettingsState.setLanguage("fr")
            compare(child("languageSelector").currentValue, "fr")
            compare(appStack.depth, 1)
            compare(appStack.currentItem, homeRoot)
            compare(homeModel.newsItems.length, 0)
            tryCompare(testSupport, "requestCount", 6)
            tryCompare(homeModel.newsItems, "length", 1)
            compare(homeModel.newsItems[0].title, "Navigation fixture français")
            compare(homeModel.newsItems[0].description, "A news story")
            verify(testSupport.requests.every(function(request) {
                return request.host.indexOf("wikipedia.org") < 0
                    || request.host.indexOf("fr.wikipedia.org") >= 0
            }))

            testSupport.clearRequests()
            SettingsState.setLanguage("en")
            compare(child("languageSelector").currentValue, "en")
            compare(homeModel.newsItems.length, 0)
            tryCompare(testSupport, "requestCount", 5)
            tryCompare(homeModel.newsItems, "length", 43)
        }
    }
}
