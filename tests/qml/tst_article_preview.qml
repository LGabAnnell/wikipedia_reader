import QtQuick
import QtQuick.Controls
import QtTest
import wikipedia_qt.ContentDisplay

Item {
    width: 500
    height: 400
    ArticlePreviewModel { id: model }
    ApplicationWindow {
        id: previewWindow
        width: 800
        height: 600
        visible: false
    }
    Component {
        id: previewComponent
        ArticlePreview { previewModel: model }
    }

    TestCase {
        name: "ArticlePreviewModelBehavior"
        when: windowShown
        property var popup: null

        function init() {
            testSupport.clearQmlWarnings()
        }

        function createPreview(windowWidth, windowHeight, anchorX, anchorY) {
            previewWindow.width = windowWidth
            previewWindow.height = windowHeight
            previewWindow.show()
            popup = createTemporaryObject(previewComponent, previewWindow.contentItem,
                                          {anchorX: anchorX, anchorY: anchorY})
            verify(popup !== null)
            tryVerify(function() { return popup.parent !== null })
            tryCompare(popup.parent, "width", windowWidth)
            tryCompare(popup.parent, "height", windowHeight)
            return popup
        }

        function verifyPlacement(below, anchorY) {
            tryVerify(function() {
                const edge = below ? popup.y : popup.y + popup.height
                return Math.abs(edge - (anchorY + (below ? 12 : -12))) < 0.5
            }, 1000, "The edge nearest the pointer must stay on the chosen side with a 12 px gap")
            verify(popup.x >= 12 && popup.x + popup.width <= popup.parent.width - 12)
            verify(popup.y >= 12 && popup.y + popup.height <= popup.parent.height - 12)
            verify(popup.height <= 420)
        }

        function summaryUrl(title, language) {
            return "https://" + language + ".wikipedia.org/api/rest_v1/page/summary/" + encodeURIComponent(title)
        }

        function test_loading_then_formatted_summary() {
            const title = "Preview fixture"
            networkFixtures.addFixture("GET", summaryUrl(title, "en"), JSON.stringify({
                title: title,
                pageid: 891,
                namespace: {id: 0},
                extract: "Preview fixture introduction",
                extract_html: "<p title='unsafe'><b>Preview</b> introduction <a href='https://bad.test'>link</a></p>",
                thumbnail: {source: "https://upload.wikimedia.org/preview.jpg"}
            }))
            model.requestPreview(title)
            compare(model.status, ArticlePreviewModel.Loading)
            tryCompare(model, "status", ArticlePreviewModel.Ready)
            compare(model.title, title)
            compare(model.pageId, 891)
            verify(model.summaryHtml.indexOf("<b>Preview</b>") >= 0)
            verify(model.summaryHtml.indexOf("href") < 0)
            compare(model.thumbnailUrl, "https://upload.wikimedia.org/preview.jpg")
            compare(testSupport.requestCount, 1)
        }

        function test_namespace_pages_are_unavailable() {
            networkFixtures.addFixture("GET", summaryUrl("Category:Books", "en"), JSON.stringify({
                title: "Category:Books",
                pageid: 892,
                namespace: {id: 14}
            }))
            model.requestPreview("Category:Books")
            tryCompare(model, "status", ArticlePreviewModel.Unavailable)
            compare(testSupport.requestCount, 1)
        }

        function test_empty_sanitized_markup_falls_back_to_escaped_extract() {
            networkFixtures.addFixture("GET", summaryUrl("Text fallback", "en"), JSON.stringify({
                title: "Text fallback",
                pageid: 893,
                namespace: {id: 0},
                extract: "Readable fallback text",
                extract_html: "<p><img src='https://invalid.example/image.png'></p>"
            }))
            model.requestPreview("Text fallback")
            tryCompare(model, "status", ArticlePreviewModel.Ready)
            verify(model.summaryHtml.indexOf("Readable fallback text") >= 0)
            verify(model.summaryHtml.indexOf("<img") < 0)
            compare(testSupport.requestCount, 1)
        }

        function test_position_stays_on_same_side_as_content_loads_data() {
            return [
                {tag: "top", windowHeight: 600, anchorX: 60, anchorY: 40, below: true},
                {tag: "both_sides_fit", windowHeight: 1400, anchorX: 60, anchorY: 800, below: true},
                {tag: "middle_above", windowHeight: 600, anchorX: 60, anchorY: 320, below: false},
                {tag: "middle_below", windowHeight: 600, anchorX: 60, anchorY: 280, below: true},
                {tag: "bottom_right", windowHeight: 600, anchorX: 799, anchorY: 580, below: false},
                {tag: "short_window_tie", windowHeight: 400, anchorX: 60, anchorY: 200, below: true}
            ]
        }

        function test_position_stays_on_same_side_as_content_loads(data) {
            const title = "Stable preview " + data.tag
            const url = summaryUrl(title, "en")
            const thumbnailUrl = "https://upload.wikimedia.org/stable-preview-" + data.tag + ".ppm"
            const extract = "A longer preview description that wraps over several lines. ".repeat(15)
            networkFixtures.addFixture("GET", url, JSON.stringify({
                title: title, pageid: 894, namespace: {id: 0},
                extract: extract, extract_html: "<p>" + extract + "</p>",
                thumbnail: {source: thumbnailUrl}
            }))
            networkFixtures.addFixture("GET", thumbnailUrl, "P3\n1 1\n255\n255 0 0\n", "image/x-portable-pixmap")
            createPreview(800, data.windowHeight, data.anchorX, data.anchorY)
            networkFixtures.deferNextReply()
            model.requestPreview(title)
            popup.open()
            tryCompare(popup, "opened", true)
            tryCompare(networkFixtures, "pendingReplyCount", 1)
            verifyPlacement(data.below, data.anchorY)
            const loadingHeight = popup.height

            // Hold the image separately so both sources of layout growth are exercised.
            networkFixtures.deferNextReply()
            networkFixtures.completeReply(url)
            tryCompare(model, "status", ArticlePreviewModel.Ready)
            tryCompare(networkFixtures, "pendingReplyCount", 1)
            const thumbnail = findChild(popup, "articlePreviewThumbnail")
            tryCompare(thumbnail, "status", Image.Loading)
            tryVerify(function() { return popup.height > loadingHeight })
            verifyPlacement(data.below, data.anchorY)

            networkFixtures.completeReply(thumbnailUrl)
            tryCompare(thumbnail, "status", Image.Ready)
            tryCompare(thumbnail, "visible", true)
            verify(waitForRendering(previewWindow.contentItem))
            verify(thumbnail.height > 0)
            const summaryScroll = findChild(popup, "articlePreviewSummaryScroll")
            tryVerify(function() { return summaryScroll.contentHeight > summaryScroll.availableHeight })
            const openButton = findChild(popup, "articlePreviewOpenButton")
            const buttonBounds = openButton.mapToItem(popup.contentItem, 0, 0)
            verify(buttonBounds.y + openButton.height <= popup.contentItem.height,
                   "Open action bottom " + (buttonBounds.y + openButton.height)
                   + " must fit within card height " + popup.contentItem.height)
            verifyPlacement(data.below, data.anchorY)
            compare(networkFixtures.requestCount, 2)
        }

        function test_resize_keeps_side_and_reopen_selects_new_side() {
            createPreview(800, 600, 790, 100)
            popup.open()
            tryCompare(popup, "opened", true)
            verifyPlacement(true, 100)

            // An above placement would have more room after this resize, but the
            // open popup must retain its side and shrink to the available space.
            previewWindow.width = 300
            previewWindow.height = 180
            tryCompare(popup.parent, "width", 300)
            tryCompare(popup.parent, "height", 180)
            tryCompare(popup, "height", 56)
            verifyPlacement(true, 100)

            popup.close()
            tryCompare(popup, "visible", false)
            previewWindow.width = 800
            previewWindow.height = 600
            tryCompare(popup.parent, "height", 600)
            popup.anchorY = 580
            popup.open()
            tryCompare(popup, "opened", true)
            verifyPlacement(false, 580)

            previewWindow.width = 300
            previewWindow.height = 590
            tryCompare(popup.parent, "width", 300)
            tryCompare(popup.parent, "height", 590)
            verifyPlacement(false, 580)
        }

        function cleanup() {
            if (popup) {
                popup.close()
                tryCompare(popup, "visible", false)
                popup = null
            }
            previewWindow.hide()
            model.clearPreview()
            testSupport.clearRequests()
            compare(testSupport.qmlWarningCount, 0, testSupport.qmlWarnings.join("\n"))
        }
    }
}

