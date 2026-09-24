import QtQuick
import QtQuick.Controls
import QtTest
import wikipedia_qt
import wikipedia_qt.ContentDisplay

Item {
    id: root
    width: 800
    height: 600

    property string fixtureTitle: "Sections fixture"
    property string articleMarkup: ""
    property var display: null

    Component {
        id: displayComponent

        ContentDisplay {
            width: root.width
            height: root.height
            articleText: root.articleMarkup
        }
    }

    TestCase {
        name: "SectionsBehavior"
        when: windowShown

        function sectionsUrl() {
            return "https://en.wikipedia.org/w/api.php?action=parse&format=json"
                    + "&prop=tocdata&page=" + encodeURIComponent(fixtureTitle)
        }

        function sectionsResponse() {
            return JSON.stringify({"parse": {"tocdata": {"sections": [
                {"line": "First section", "tocLevel": 1, "anchor": "First_section", "index": "1"},
                {"line": "Second section", "tocLevel": 1, "anchor": "Second_section", "index": "2"}
            ]}}})
        }

        function articleHtml() {
            let html = "<p>Introduction before the sections.</p>"
                    + "<h2><a name=\"First_section\">First section</a></h2>"
            for (let i = 0; i < 24; ++i)
                html += "<p>First section paragraph " + i
                        + ". This text provides enough article content to scroll through.</p>"
            html += "<h2><a name=\"Second_section\">Second section</a></h2>"
            for (let j = 0; j < 24; ++j)
                html += "<p>Second section paragraph " + j
                        + ". This text keeps the second heading within the scrollable article.</p>"
            return html
        }

        function resetState() {
            if (display) {
                display.destroy()
                display = null
            }
            GlobalState.setCurrentPageFromData("", "", "")
            GlobalState.clearErrorMessage()
            GlobalState.setIsLoading(false)
            GlobalState.setSections([])
            GlobalState.setLoadingSections(false)
            GlobalState.setCurrentSectionIndex(-1)
        }

        function init() {
            resetState()
            testSupport.clearRequests()
            testSupport.clearQmlWarnings()
        }

        function cleanup() {
            while (networkFixtures.pendingReplyCount > 0)
                networkFixtures.completeNextReply()
            if (display) {
                display.destroy()
                display = null
            }
            GlobalState.setCurrentPageFromData("", "", "")
            GlobalState.clearErrorMessage()
            GlobalState.setIsLoading(false)
            GlobalState.setSections([])
            GlobalState.setLoadingSections(false)
            GlobalState.setCurrentSectionIndex(-1)
            compare(testSupport.qmlWarningCount, 0, testSupport.qmlWarnings.join("\n"))
        }

        function createDisplay(statusCode) {
            networkFixtures.addFixture("GET", sectionsUrl(),
                                       statusCode === undefined ? sectionsResponse() : "section request failed",
                                       statusCode === undefined ? "application/json" : "text/plain",
                                       statusCode === undefined ? 200 : statusCode)
            root.articleMarkup = articleHtml()
            GlobalState.setCurrentPageFromData(fixtureTitle, "Sections fixture extract", "")
            display = displayComponent.createObject(root)
            verify(display !== null, "ContentDisplay must be created")
            wait(0)
            return display
        }

        function child(name) {
            const item = findChild(root, name)
            verify(item !== null, "Missing object named " + name)
            return item
        }

        function expandPanel() {
            const toggle = child("sectionsToggleButton")
            mouseClick(toggle)
            tryCompare(child("sectionsPanel"), "collapsed", false)
            tryCompare(display, "sectionResizeInProgress", false)
        }

        function test_deferred_loading_populates_sections() {
            networkFixtures.deferNextReply()
            createDisplay()
            expandPanel()

            const loading = child("sectionsLoadingIndicator")
            const list = child("sectionsList")
            tryCompare(networkFixtures, "pendingReplyCount", 1)
            compare(testSupport.requestCount, 1)
            compare(loading.visible, true)

            networkFixtures.completeNextReply()
            tryCompare(loading, "visible", false)
            tryCompare(list, "count", 2)
            compare(child("sectionEntry-1").contentItem.text, "First section")
            compare(child("sectionEntry-2").contentItem.text, "Second section")
        }

        function test_clicking_section_scrolls_to_its_heading() {
            createDisplay()
            expandPanel()
            tryCompare(child("sectionsList"), "count", 2)

            const body = child("articleBody")
            const scroll = child("articleScrollView")
            mouseClick(child("sectionEntry-2"))
            tryVerify(function() { return body.cursorPosition > 0 })
            tryVerify(function() { return scroll.contentItem.contentY > 0 })

            const headingRect = body.positionToRectangle(body.cursorPosition)
            const headingY = body.mapToItem(scroll, 0, headingRect.y).y
            verify(headingY >= -5 && headingY < scroll.height,
                   "Selected heading should be visible in the article viewport; y="
                   + headingY + ", scrollY=" + scroll.contentItem.contentY
                   + ", viewportHeight=" + scroll.height)
        }

        function test_scroll_updates_active_section_and_highlight() {
            createDisplay()
            expandPanel()
            tryCompare(child("sectionsList"), "count", 2)

            const scroll = child("articleScrollView")
            scroll.contentItem.contentY = 150
            compare(scroll.contentHeight > scroll.height, true)
            compare(scroll.contentItem.contentY, 150)
            tryCompare(GlobalState, "currentSectionIndex", 1, 5000,
                       "Expected first section at scrollY=150; contentHeight=" + scroll.contentHeight
                       + ", viewportHeight=" + scroll.height
                       + ", actual scrollY=" + scroll.contentItem.contentY)
            tryVerify(function() { return child("sectionEntry-1").highlighted })

            const laterScrollY = Math.min(1500, scroll.contentHeight - scroll.height)
            scroll.contentItem.contentY = laterScrollY
            tryCompare(GlobalState, "currentSectionIndex", 2)
            scroll.contentItem.contentY = 150
            tryCompare(GlobalState, "currentSectionIndex", 1)
            scroll.contentItem.contentY = laterScrollY
            tryCompare(GlobalState, "currentSectionIndex", 2)
            tryVerify(function() { return child("sectionEntry-2").highlighted })
            compare(child("sectionEntry-1").highlighted, false)
        }

        function test_empty_sections_show_empty_message() {
            networkFixtures.addFixture("GET", sectionsUrl(),
                                       JSON.stringify({"parse": {"tocdata": {"sections": []}}}))
            root.articleMarkup = articleHtml()
            GlobalState.setCurrentPageFromData(fixtureTitle, "Sections fixture extract", "")
            display = displayComponent.createObject(root)
            verify(display !== null)
            expandPanel()

            tryCompare(child("sectionsList"), "count", 0)
            tryCompare(child("sectionsEmptyMessage"), "visible", true)
            compare(child("sectionsEmptyMessage").text, "No sections found")
            compare(child("sectionsLoadingIndicator").visible, false)
        }

        function test_request_error_stops_loading_and_displays_error() {
            createDisplay(500)
            expandPanel()

            const loading = child("sectionsLoadingIndicator")
            const error = child("sectionsErrorMessage")
            tryCompare(loading, "visible", false)
            tryCompare(error, "visible", true)
            compare(error.text, "HTTP 500")
        }
    }
}
