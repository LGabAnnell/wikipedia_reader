import QtQuick
import QtTest
import wikipedia_qt
import wikipedia_qt.ContentDisplay 1.0

Item {
    id: root
    width: 800
    height: 600

    Component {
        id: contentDisplayComponent

        ContentDisplay {
            width: root.width
            height: root.height
        }
    }

    TestCase {
        name: "InlineImages"
        when: windowShown

        property var display: null

        function init() {
            testSupport.clearRequests()
            testSupport.clearQmlWarnings()
            display = contentDisplayComponent.createObject(root)
            verify(display !== null, "ContentDisplay must be created")
            wait(0)
        }

        function cleanup() {
            if (display)
                display.destroy()
            display = null
            compare(testSupport.qmlWarningCount, 0, testSupport.qmlWarnings.join("\n"))
        }

        function test_thumb_host_thumbnail_converts_to_original() {
            // Current MediaWiki parse output serves thumbnails from
            // thumb.wikimedia.org with tracking query parameters.
            const thumbnail = "https://thumb.wikimedia.org/wikipedia/commons/thumb/2/28/Albert_Einstein_Head_cleaned.jpg/250px-Albert_Einstein_Head_cleaned.jpg?utm_source=en.wikipedia.org&utm_campaign=parser&utm_content=thumbnail";
            compare(display.fullResolutionImageUrl(thumbnail),
                    "https://upload.wikimedia.org/wikipedia/commons/2/28/Albert_Einstein_Head_cleaned.jpg")
        }

        function test_upload_host_thumbnail_converts_to_original() {
            const thumbnail = "https://upload.wikimedia.org/wikipedia/commons/thumb/6/66/Hermann_einstein.jpg/250px-Hermann_einstein.jpg";
            compare(display.fullResolutionImageUrl(thumbnail),
                    "https://upload.wikimedia.org/wikipedia/commons/6/66/Hermann_einstein.jpg")
        }

        function test_lossy_page_thumbnail_converts_to_original() {
            const thumbnail = "https://thumb.wikimedia.org/wikipedia/commons/thumb/7/77/Document.tif/lossy-page1-330px-Document.tif.jpg";
            compare(display.fullResolutionImageUrl(thumbnail),
                    "https://upload.wikimedia.org/wikipedia/commons/7/77/Document.tif")
        }

        function test_svg_thumbnail_converts_to_original() {
            const thumbnail = "https://thumb.wikimedia.org/wikipedia/commons/thumb/d/d7/Signature.svg/250px-Signature.svg.png";
            compare(display.fullResolutionImageUrl(thumbnail),
                    "https://upload.wikimedia.org/wikipedia/commons/d/d7/Signature.svg")
        }

        function test_non_thumbnail_urls_are_unchanged() {
            const original = "https://upload.wikimedia.org/wikipedia/commons/2/28/Albert_Einstein_Head_cleaned.jpg";
            compare(display.fullResolutionImageUrl(original), original)

            const external = "https://example.com/images/thumb.jpg";
            compare(display.fullResolutionImageUrl(external), external)
        }

        function test_inline_image_link_carries_full_resolution_url() {
            const html = "<a href=\"/wiki/File:Foo.jpg\" class=\"mw-file-description\">"
                    + "<img alt=\"Foo\" src=\"//thumb.wikimedia.org/wikipedia/commons/thumb/2/28/Foo.jpg/250px-Foo.jpg?utm_source=en.wikipedia.org&amp;utm_campaign=parser&amp;utm_content=thumbnail\" width=\"250\" height=\"309\" />"
                    + "</a>";
            const rendered = display.renderArticleText(html);

            const hrefMatch = rendered.match(/href="([^"]*)"/);
            verify(hrefMatch !== null, "Rendered anchor must keep an href")
            const imageLinkPrefix = "wikipedia-image:";
            verify(hrefMatch[1].startsWith(imageLinkPrefix),
                   "Image anchor must link to the image view, got: " + hrefMatch[1])

            const image = JSON.parse(decodeURIComponent(
                                          hrefMatch[1].substring(imageLinkPrefix.length)));
            compare(image.url,
                    "https://upload.wikimedia.org/wikipedia/commons/2/28/Foo.jpg")
            compare(image.description, "Foo")
        }
    }
}
