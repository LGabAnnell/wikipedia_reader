import QtQuick
import QtQuick.Controls
import QtTest
import wikipedia_qt
import wikipedia_qt.ImageDisplay 1.0

Item {
    id: root
    width: 800
    height: 600

    Component {
        id: galleryComponent

        ImageGallery {
            width: root.width
            height: root.height
        }
    }

    Component {
        id: imageViewComponent

        ImageView {
            width: root.width
            height: root.height
        }
    }

    StackView {
        id: stackView
        objectName: "imageTestStack"
        anchors.fill: parent
        z: 1
    }

    Component.onCompleted: {
        NavigationState.addView(Constants.imageView, imageViewComponent)
    }

    Connections {
        target: NavigationState
        function onPushView(view) {
            stackView.push(view)
        }
    }

    TestCase {
        name: "ImageGalleryBehavior"
        when: windowShown

        readonly property int articleId: 301
        readonly property string articleTitle: "Gallery fixture article"
        readonly property string imageUrl: "https://upload.wikimedia.org/wikipedia/commons/fixture.png"
        readonly property string imageDescription: "A fixture caption"

        property var gallery: null

        function pageQueryUrl(pageId) {
            return "https://en.wikipedia.org/w/api.php?action=query&format=json"
                    + "&prop=extracts%7Cimages&pageids=" + pageId
                    + "&explaintext=1&imlimit=50"
        }

        function parseUrl(pageId) {
            return "https://en.wikipedia.org/w/api.php?action=parse&disableeditsection=true"
                    + "&format=json&formatversion=2&pageid=" + pageId + "&prop=text"
        }

        function imageInfoUrl() {
            return "https://en.wikipedia.org/w/api.php?action=query&format=json&prop=imageinfo"
                    + "&iiextmetadata=ImageDescription&iiprop=url%7Cextmetadata"
                    + "&titles=File%3AFixture.png"
        }

        function configureArticleFixture() {
            var pages = {}
            pages[articleId] = {
                "pageid": articleId,
                "title": articleTitle,
                "extract": "Gallery fixture extract",
                "images": [{"title": "File:Fixture.png"}]
            }
            networkFixtures.addFixture("GET", pageQueryUrl(articleId),
                                       JSON.stringify({"query": {"pages": pages}}))
            networkFixtures.addFixture("GET", parseUrl(articleId),
                                       JSON.stringify({"parse": {
                                           "title": articleTitle,
                                           "pageid": articleId,
                                           "text": "<p>Gallery fixture article</p>"
                                       }}))

            var imagePages = {}
            imagePages["42"] = {
                "imageinfo": [{
                    "url": imageUrl,
                    "extmetadata": {"ImageDescription": {"value": imageDescription}}
                }]
            }
            networkFixtures.addFixture("GET", imageInfoUrl(),
                                       JSON.stringify({"query": {"pages": imagePages}}))
        }

        function init() {
            testSupport.clearRequests()
            testSupport.clearQmlWarnings()
            HistoryState.clearHistory()
            GlobalState.currentImageUrl = ""
            GlobalState.currentImageDescription = ""
            NavigationState.setStackView(stackView)
            configureArticleFixture()
            gallery = null
            compare(stackView.depth, 0)
        }

        function cleanup() {
            while (networkFixtures.pendingReplyCount > 0)
                networkFixtures.completeNextReply()
            if (stackView.depth > 0)
                stackView.pop()
            if (gallery)
                gallery.destroy()
            gallery = null
            GlobalState.currentImageUrl = ""
            GlobalState.currentImageDescription = ""
            HistoryState.clearHistory()
            NavigationState.setStackView(null)
            compare(testSupport.qmlWarningCount, 0, testSupport.qmlWarnings.join("\n"))
        }

        function child(name) {
            const item = findChild(root, name)
            verify(item !== null, "Missing object named " + name)
            return item
        }

        function test_selecting_gallery_image_shows_caption() {
            GlobalState.loadArticleByPageId(articleId)
            tryCompare(GlobalState, "isLoading", true)
            tryCompare(GlobalState, "isLoading", false)
            compare(GlobalState.currentPageId, articleId)
            compare(HistoryState.history.length, 1)
            compare(HistoryState.history[0].pageId, articleId)

            gallery = galleryComponent.createObject(root)
            verify(gallery !== null, "ImageGallery must be created")
            // The gallery's metadata and thumbnail each load through the fake
            // transport; the fullscreen view requests the selected PNG too.
            tryCompare(networkFixtures, "requestCount", 5)

            const cell = child("imageGalleryCell-0")
            mouseClick(cell)
            tryCompare(GlobalState, "currentImageUrl", imageUrl)
            compare(GlobalState.currentImageDescription, imageDescription)
            tryCompare(stackView, "depth", 1)

            const fullImage = child("fullScreenImage")
            tryCompare(fullImage, "status", Image.Ready)
            compare(fullImage.source.toString(), imageUrl)

            const captionOverlay = child("imageCaptionOverlay")
            tryCompare(captionOverlay, "visible", true)
            const caption = child("imageCaptionText")
            compare(caption.text, imageDescription)
            compare(testSupport.requestCount, 6)
        }
    }
}
