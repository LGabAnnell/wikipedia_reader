import QtQuick
import QtTest
import wikipedia_qt

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
                const featuredUrl = "https://api.wikimedia.org/feed/v1/wikipedia/" + lang + "/featured/" + date
                const onThisDayUrl = "https://api.wikimedia.org/feed/v1/wikipedia/" + lang + "/onthisday/all/" + monthDay
                const randomTitleUrl = "https://" + lang + ".wikipedia.org/api/rest_v1/page/random/title"
                const pageImagesUrl = "https://" + lang + ".wikipedia.org/w/api.php?action=query&format=json&prop=extracts%7Cimages&pageids=1&explaintext=1&imlimit=50"
                const title = lang === "en" ? "Navigation fixture" : "Navigation fixture français"
                networkFixtures.addFixture("GET", featuredUrl,
                                           JSON.stringify({"tfa":{"title":title,"extract":"Fixture article","pageid":1},"mostread":{"articles":[{"title":title,"extract":"News item","pageid":1,"content_urls":{"desktop":{"page":"https://" + lang + ".wikipedia.org/wiki/Fixture"}}}]}}))
                networkFixtures.addFixture("GET", onThisDayUrl, '{"selected":[]}')
                networkFixtures.addFixture("GET", randomTitleUrl, '{}')
                networkFixtures.addFixture("GET", pageImagesUrl,
                                           JSON.stringify({"query":{"pages":{"1":{"pageid":1,"title":title,"extract":"","images":[]}}}}))
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
            compare(testSupport.qmlWarningCount, 0, testSupport.qmlWarnings.join("\n"))
        }

        function child(name) {
            const item = findChild(appWindow, name)
            verify(item !== null, "Missing object named " + name)
            return item
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

        function test_mouse_back_button_pops_one_view() {
            mouseClick(child("searchButton"))
            tryCompare(appStack, "depth", 2)
            tryVerify(function() { return !appStack.busy })

            verify(testSupport.sendBackButtonPress(appWindow), "Back button event should be handled")
            tryCompare(appStack, "depth", 1)
            compare(appStack.currentItem, homeRoot)
        }

        function test_language_change_keeps_home_and_refreshes_its_data() {
            const homeModel = child("homeModel")
            tryCompare(homeModel.newsItems, "length", 1)
            testSupport.clearRequests()
            SettingsState.setLanguage("fr")
            compare(appStack.depth, 1)
            compare(appStack.currentItem, homeRoot)
            compare(homeModel.newsItems.length, 0)
            tryCompare(testSupport, "requestCount", 5)
            tryCompare(homeModel.newsItems, "length", 1)
            verify(testSupport.requests.every(function(request) {
                return request.host.indexOf("wikipedia.org") < 0
                    || request.host.indexOf("fr.wikipedia.org") >= 0
            }))

            testSupport.clearRequests()
            SettingsState.setLanguage("en")
            compare(homeModel.newsItems.length, 0)
            tryCompare(testSupport, "requestCount", 5)
            tryCompare(homeModel.newsItems, "length", 1)
        }
    }
}
