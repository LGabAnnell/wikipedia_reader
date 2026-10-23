import QtQuick
import QtTest
import wikipedia_qt.ContentDisplay

Item {
    width: 500
    height: 400
    ArticlePreviewModel { id: model }

    TestCase {
        name: "ArticlePreviewModelBehavior"
        when: windowShown

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

        function cleanup() {
            model.clearPreview()
            testSupport.clearRequests()
        }
    }
}

