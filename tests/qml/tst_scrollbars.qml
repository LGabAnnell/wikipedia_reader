import QtQuick
import QtQuick.Controls
import QtTest
import wikipedia_qt.ContentDisplay

Item {
    id: root
    width: 800
    height: 600

    Component {
        id: articleComponent
        ContentDisplay {
            property bool mirroredLayout: false
            LayoutMirroring.enabled: mirroredLayout
            LayoutMirroring.childrenInherit: true
            width: 800
            height: 600
            articleTitle: "Scrollbar fixture"
            articleText: "<p>Scrollable article content.</p>".repeat(150)
        }
    }

    TestCase {
        name: "ArticleScrollbars"
        when: windowShown

        function init() {
            testSupport.clearRequests()
            testSupport.clearQmlWarnings()
            networkFixtures.addFixture("GET",
                "https://en.wikipedia.org/w/api.php?action=parse&format=json&prop=tocdata&page=Scrollbar%20fixture",
                '{"parse":{"tocdata":{"sections":[]}}}')
        }

        function cleanup() {
            compare(testSupport.qmlWarningCount, 0, testSupport.qmlWarnings.join("\n"))
            compare(testSupport.requestCount, 1)
        }

        function test_resize_and_drag_data() {
            return [{ tag: "left-to-right", mirrored: false },
                    { tag: "right-to-left", mirrored: true }]
        }

        function test_resize_and_drag(data) {
            const article = createTemporaryObject(articleComponent, root,
                                                  { mirroredLayout: data.mirrored })
            verify(article !== null)
            const scroll = findChild(article, "articleScrollView")
            verify(scroll !== null)
            const bar = scroll.ScrollBar.vertical

            for (const width of [800, 420, 1000, 600]) {
                article.width = width
                waitForRendering(article)
                verify(bar.visible)
                verify(bar.interactive)
                verify(bar.size > 0 && bar.size < 1)
                const edge = bar.mapToItem(scroll, data.mirrored ? 0 : bar.width, 0).x
                const expected = data.mirrored ? 0 : scroll.width
                verify(Math.abs(edge - expected) < 1,
                       "Scrollbar edge " + edge + " must match viewport edge " + expected)
            }

            mousePress(bar, bar.width / 2, bar.height * bar.size / 2)
            mouseMove(bar, bar.width / 2, bar.height / 2)
            mouseRelease(bar, bar.width / 2, bar.height / 2)
            tryVerify(function() { return scroll.contentItem.contentY > 0 })
        }
    }
}
