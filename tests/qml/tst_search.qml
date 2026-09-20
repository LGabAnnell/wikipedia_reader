import QtQuick
import QtQuick.Controls
import QtTest
import wikipedia_qt

Item {
    id: root
    width: 800
    height: 600

    Component {
        id: searchScreenComponent

        SearchScreen {
            width: root.width
            height: root.height
        }
    }

    TestCase {
        name: "SearchBehavior"
        when: windowShown

        readonly property string fixtureUrl: "https://en.wikipedia.org/w/api.php?action=query&format=json&list=search&srsearch=fixture&srlimit=10"
        readonly property string emptyUrl: "https://en.wikipedia.org/w/api.php?action=query&format=json&list=search&srsearch=empty&srlimit=10"
        readonly property string failureUrl: "https://en.wikipedia.org/w/api.php?action=query&format=json&list=search&srsearch=failure&srlimit=10"

        property var screen: null

        function init() {
            testSupport.clearRequests()
            testSupport.clearQmlWarnings()
            screen = searchScreenComponent.createObject(root)
            verify(screen !== null, "SearchScreen must be created")
            wait(0)
        }

        function cleanup() {
            while (networkFixtures.pendingReplyCount > 0)
                networkFixtures.completeNextReply()
            if (screen)
                screen.destroy()
            screen = null
            compare(testSupport.qmlWarningCount, 0, testSupport.qmlWarnings.join("\n"))
        }

        function child(name) {
            const item = findChild(screen, name)
            verify(item !== null, "Missing object named " + name)
            return item
        }

        function deferAndSubmit(input, query, button) {
            networkFixtures.deferNextReply()
            input.text = query
            verify(button.enabled)
            mouseClick(button)
            tryCompare(networkFixtures, "pendingReplyCount", 1)
            compare(testSupport.requestCount, 1)
        }

        function test_empty_input_does_not_submit() {
            const input = child("searchInput")
            const button = child("searchButton")
            compare(button.enabled, false)

            input.forceActiveFocus()
            keyClick(Qt.Key_Return)
            compare(testSupport.requestCount, 0)

            input.text = "   \t  "
            compare(button.enabled, false)
            keyClick(Qt.Key_Return)
            compare(testSupport.requestCount, 0)
        }

        function test_mouse_submission_populates_results() {
            const input = child("searchInput")
            const button = child("searchButton")
            const busy = child("searchLoadingIndicator")
            const list = child("searchResultsList")

            deferAndSubmit(input, "  fixture  ", button)
            compare(input.enabled, false)
            compare(button.enabled, false)
            compare(busy.visible, true)
            verify(testSupport.requests[0].query.indexOf("srsearch=fixture") >= 0)

            networkFixtures.completeNextReply()
            tryCompare(busy, "visible", false)
            compare(input.enabled, true)
            compare(button.enabled, true)
            tryCompare(list, "count", 1)

            const result = child("searchResultContent-0")
            compare(result.resultTitle, "Fixture article")
            compare(result.resultSnippet, "Committed fixture result")
        }

        function test_enter_submission_shows_empty_feedback() {
            const input = child("searchInput")
            const button = child("searchButton")
            const busy = child("searchLoadingIndicator")
            const status = child("searchStatusLabel")
            const list = child("searchResultsList")

            networkFixtures.deferNextReply()
            input.text = "empty"
            input.forceActiveFocus()
            keyClick(Qt.Key_Return)
            tryCompare(networkFixtures, "pendingReplyCount", 1)
            compare(testSupport.requestCount, 1)
            compare(busy.visible, true)
            compare(button.enabled, false)

            networkFixtures.completeNextReply()
            tryCompare(busy, "visible", false)
            tryCompare(status, "visible", true)
            compare(status.text, "No results found.")
            compare(list.count, 0)
            compare(button.enabled, true)
        }

        function test_failure_then_retry_recovers() {
            networkFixtures.addFixture("GET", failureUrl, "failure", "text/plain", 500)
            const input = child("searchInput")
            const button = child("searchButton")
            const busy = child("searchLoadingIndicator")
            const status = child("searchStatusLabel")
            const list = child("searchResultsList")

            deferAndSubmit(input, "failure", button)
            networkFixtures.completeNextReply()
            tryCompare(busy, "visible", false)
            tryCompare(status, "visible", true)
            compare(status.text, "HTTP 500")
            compare(input.enabled, true)
            compare(button.enabled, true)

            networkFixtures.addFixture("GET", failureUrl,
                                       '{"query":{"search":[{"title":"Recovered article","snippet":"Retry succeeded","pageid":9}]}}')
            networkFixtures.deferNextReply()
            mouseClick(button)
            tryCompare(networkFixtures, "pendingReplyCount", 1)
            compare(testSupport.requestCount, 2)
            compare(busy.visible, true)
            compare(status.visible, false)
            compare(status.text, "")

            networkFixtures.completeNextReply()
            tryCompare(list, "count", 1)
            tryCompare(busy, "visible", false)
            compare(status.visible, false)
            const result = child("searchResultContent-0")
            compare(result.resultTitle, "Recovered article")
            compare(result.resultSnippet, "Retry succeeded")
        }
    }
}
