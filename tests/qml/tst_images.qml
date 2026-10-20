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
        id: galleryModelComponent
        ImageHomeModel {}
    }

    Component {
        id: fullscreenComponent
        ImageFullscreenWindow {}
    }

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
            onBackRequested: stackView.pop()
        }
    }

    StackView {
        id: stackView
        objectName: "imageTestStack"
        anchors.fill: parent
        z: 1
    }

    Connections {
        target: NavigationState
        function onPushView(view) {
            stackView.push(view)
        }
    }

    // The article underneath the viewer also registers Escape (to hide Find).
    Shortcut {
        sequence: "Escape"
        onActivated: root.underlyingEscapeCount++
    }

    property int underlyingEscapeCount: 0

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

        function imageInfoUrl(title, details) {
            return "https://en.wikipedia.org/w/api.php?action=query&format=json&prop=imageinfo"
                    + "&iiprop=url%7Cextmetadata%7Cmime"
                    + (details ? "&iiextmetadatafilter=ImageDescription" : "")
                    + "&iiurlwidth=" + (details ? "2048" : "330")
                    + "&iiurlheight=" + (details ? "2048" : "330")
                    + "&titles=" + encodeURIComponent(title || "File:Fixture.svg")
        }

        function configureArticleFixture() {
            var pages = {}
            pages[articleId] = {
                "pageid": articleId,
                "title": articleTitle,
                "extract": "Gallery fixture extract",
                "images": [{"title": "File:Fixture.svg"}]
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
                "title": "File:Fixture.svg",
                "imageinfo": [{
                    // This original has no transport fixture: loading it is a
                    // failure. The gallery and viewer must request the PNG.
                    "url": "https://upload.wikimedia.org/wikipedia/commons/empty-masked.svg",
                    "mime": "image/svg+xml",
                    "thumburl": imageUrl,
                    "extmetadata": {"ImageDescription": {"value": imageDescription}}
                }]
            }
            networkFixtures.addFixture("GET", imageInfoUrl(),
                                       JSON.stringify({"query": {"pages": imagePages}}))
            imagePages["42"].imageinfo[0].thumburl = imageUrl + "#fullscreen"
            networkFixtures.addFixture("GET", imageInfoUrl("File:Fixture.svg", true),
                                       JSON.stringify({query: {pages: imagePages}}))
        }

        function configureMultiImageArticleFixture(pageId) {
            const titleA = "File:Gallery A.png"
            const titleB = "File:Gallery B.png"
            var pages = {}
            pages[pageId] = {
                pageid: pageId,
                title: "Two image gallery fixture",
                extract: "Gallery fixture extract",
                images: [{title: titleA}, {title: titleB}]
            }
            networkFixtures.addFixture("GET", pageQueryUrl(pageId), JSON.stringify({query: {pages: pages}}))
            networkFixtures.addFixture("GET", parseUrl(pageId), JSON.stringify({parse: {
                title: "Two image gallery fixture",
                pageid: pageId,
                text: "<p>Two image gallery fixture</p>"
            }}))

            function metadata(title, description, imageUrl) {
                return JSON.stringify({query: {pages: {"42": {
                    title: title,
                    imageinfo: [{url: imageUrl, mime: "image/png", thumburl: imageUrl,
                                 extmetadata: {ImageDescription: {value: description}}}]
                }}}})
            }
            networkFixtures.addFixture("GET", imageInfoUrl(titleA), metadata(titleA, "Caption A", imageUrl))
            networkFixtures.addFixture("GET", imageInfoUrl(titleA, true), metadata(titleA, "Caption A", imageUrl + "#full-a"))
            networkFixtures.addFixture("GET", imageInfoUrl(titleB), metadata(titleB, "Caption B", imageUrl))
            networkFixtures.addFixture("GET", imageInfoUrl(titleB, true), metadata(titleB, "Caption B", imageUrl + "#full-b"))
            networkFixtures.addFixture("GET", imageInfoUrl(titleA + "|" + titleB), JSON.stringify({query: {pages: {
                "42": {title: titleA, imageinfo: [{url: imageUrl, mime: "image/png", thumburl: imageUrl,
                                                     extmetadata: {ImageDescription: {value: "Caption A"}}}]},
                "43": {title: titleB, imageinfo: [{url: imageUrl, mime: "image/png", thumburl: imageUrl,
                                                     extmetadata: {ImageDescription: {value: "Caption B"}}}]}
            }}}))
        }

        function init() {
            testSupport.clearRequests()
            testSupport.clearQmlWarnings()
            HistoryState.clearHistory()
            ImageSelectionState.selectImage("", "")
            NavigationState.setStackView(stackView)
            NavigationState.addView(Constants.imageView, imageViewComponent)
            configureArticleFixture()
            gallery = null
            compare(stackView.depth, 0)
        }

        function cleanup() {
            while (networkFixtures.pendingReplyCount > 0)
                networkFixtures.completeNextReply()
            if (stackView.depth > 0)
                stackView.clear(StackView.Immediate)
            if (gallery)
                gallery.destroy()
            gallery = null
            ImageSelectionState.selectImage("", "")
            HistoryState.clearHistory()
            NavigationState.setStackView(null)
            compare(testSupport.qmlWarningCount, 0, testSupport.qmlWarnings.join("\n"))
        }

        function child(name) {
            const item = findChild(root, name)
            verify(item !== null, "Missing object named " + name)
            return item
        }

        function apiRequestCount() {
            return testSupport.requests.filter(function(request) {
                return request.url.indexOf("https://en.wikipedia.org/w/api.php") === 0
            }).length
        }

        function test_queued_back_does_not_close_reopened_viewer() {
            const viewer = createTemporaryObject(fullscreenComponent, root)
            verify(viewer !== null)
            viewer.showFullScreen()
            tryCompare(viewer, "active", true)
            verify(testSupport.sendBackButtonPress(viewer))
            verify(testSupport.sendBackButtonRelease(viewer))
            compare(viewer.visible, true, "Closing must wait for mouse release dispatch to finish")

            // Reopen synchronously, before the queued Back callback can run.
            viewer.close()
            viewer.showFullScreen()
            let callbackProcessed = false
            Qt.callLater(function() { callbackProcessed = true })
            tryVerify(function() { return callbackProcessed })
            compare(viewer.visible, true, "An old click must not close the reopened viewer")
            viewer.close()
        }

        function test_arrow_shortcuts_navigate_gallery_in_both_viewers() {
            const urls = [imageUrl + "#one", imageUrl + "#two", imageUrl + "#three"]
            const captions = ["First", "Second", "Third"]
            ImageSelectionState.selectGalleryImage(urls, captions, [], 0)

            const embedded = imageViewComponent.createObject(root)
            verify(embedded !== null)
            stackView.push(embedded, StackView.Immediate)
            embedded.forceActiveFocus()
            verify(testSupport.sendKeyClick(root.Window.window, Qt.Key_Right))
            tryCompare(ImageSelectionState, "currentImageUrl", urls[1])
            compare(ImageSelectionState.currentImageDescription, "Second")
            child("imageBackButton").forceActiveFocus()
            verify(testSupport.sendKeyClick(root.Window.window, Qt.Key_Right))
            tryCompare(ImageSelectionState, "currentImageUrl", urls[2])
            const depth = stackView.depth
            verify(testSupport.sendKeyClick(root.Window.window, Qt.Key_Right))
            compare(ImageSelectionState.currentImageUrl, urls[2])
            compare(stackView.depth, depth)
            const cover = Qt.createQmlObject("import QtQuick; Item {}", root)
            stackView.push(cover, StackView.Immediate)
            verify(testSupport.sendKeyClick(root.Window.window, Qt.Key_Left))
            compare(ImageSelectionState.currentImageUrl, urls[2], "A covered viewer must not handle arrows")
            stackView.pop(StackView.Immediate)
            verify(testSupport.sendKeyClick(root.Window.window, Qt.Key_Left))
            tryCompare(ImageSelectionState, "currentImageUrl", urls[1])
            stackView.clear(StackView.Immediate)
            embedded.destroy()
            cover.destroy()

            const fullscreen = createTemporaryObject(fullscreenComponent, root)
            verify(fullscreen !== null)
            ImageSelectionState.selectGalleryImage(urls, captions, [], 0)
            fullscreen.showFullScreen()
            tryCompare(fullscreen, "active", true)
            const closeButton = child("imageBackButton")
            closeButton.forceActiveFocus()
            verify(testSupport.sendKeyClick(fullscreen, Qt.Key_Right))
            tryCompare(ImageSelectionState, "currentImageUrl", urls[1])
            compare(ImageSelectionState.currentImageDescription, "Second")
            verify(testSupport.sendKeyClick(fullscreen, Qt.Key_Left))
            tryCompare(ImageSelectionState, "currentImageUrl", urls[0])
            compare(stackView.depth, 0)
            fullscreen.close()

            ImageSelectionState.selectImage(imageUrl + "#inline", "Inline")
            const inlineView = imageViewComponent.createObject(root)
            verify(inlineView !== null)
            stackView.push(inlineView, StackView.Immediate)
            verify(testSupport.sendKeyClick(root.Window.window, Qt.Key_Right))
            compare(ImageSelectionState.currentImageUrl, imageUrl + "#inline")
            stackView.clear(StackView.Immediate)
            inlineView.destroy()
        }

        function test_gallery_click_arrow_navigation_data() {
            return [
                {tag: "fullscreen", fullscreen: true},
                {tag: "embedded", fullscreen: false}
            ]
        }

        function test_gallery_click_arrow_navigation(data) {
            const pageId = 304
            configureMultiImageArticleFixture(pageId)
            ArticleState.loadArticleByPageId(pageId)
            tryCompare(ArticleState, "isLoading", false)
            compare(ArticleState.currentPageId, pageId)

            gallery = galleryComponent.createObject(root)
            verify(gallery !== null)
            tryVerify(function() { return findChild(root, "imageGalleryCell-1") !== null })
            const toggle = child("imageFullscreenSwitch")
            const originalFullscreen = toggle.checked
            if (toggle.checked !== data.fullscreen)
                toggle.checked = data.fullscreen
            compare(toggle.checked, data.fullscreen)
            const firstCell = child("imageGalleryCell-0")
            const secondCell = child("imageGalleryCell-1")
            verify(waitForRendering(firstCell))
            verify(waitForRendering(secondCell))
            mouseClick(firstCell)

            const viewer = child("imageFullscreenWindow")
            if (data.fullscreen) {
                tryCompare(viewer, "visibility", Window.FullScreen)
            } else {
                tryCompare(stackView, "depth", 1)
                tryCompare(stackView, "busy", false)
                compare(viewer.visible, false)
            }
            tryCompare(ImageSelectionState, "currentImageUrl", imageUrl + "#full-a")
            compare(ImageSelectionState.currentImageDescription, "Caption A")
            const viewWindow = data.fullscreen ? viewer : root.Window.window
            viewWindow.requestActivate()
            tryCompare(viewWindow, "active", true)
            if (!data.fullscreen) {
                stackView.currentItem.forceActiveFocus()
                findChild(stackView.currentItem, "imageBackButton").forceActiveFocus()
            } else {
                child("imageBackButton").forceActiveFocus()
            }

            verify(testSupport.sendKeyClick(viewWindow, Qt.Key_Right))
            tryCompare(ImageSelectionState, "currentImageUrl", imageUrl + "#full-b")
            compare(ImageSelectionState.currentImageDescription, "Caption B")
            verify(testSupport.sendKeyClick(viewWindow, Qt.Key_Right))
            compare(ImageSelectionState.currentImageUrl, imageUrl + "#full-b", "Right stops at the last image")
            verify(testSupport.sendKeyClick(viewWindow, Qt.Key_Left))
            tryCompare(ImageSelectionState, "currentImageUrl", imageUrl + "#full-a")
            verify(testSupport.sendKeyClick(viewWindow, Qt.Key_Left))
            compare(ImageSelectionState.currentImageUrl, imageUrl + "#full-a", "Left stops at the first image")

            if (data.fullscreen) {
                viewer.close()
            } else {
                stackView.pop(StackView.Immediate)
            }
            if (toggle.checked !== originalFullscreen)
                mouseClick(toggle)
            compare(toggle.checked, originalFullscreen)
        }

        function test_stale_image_details_do_not_replace_new_selection() {
            function details(title, url, description) {
                return JSON.stringify({query: {pages: {"42": {title: title, imageinfo: [{
                    url: url, thumburl: url, mime: "image/png",
                    extmetadata: {ImageDescription: {value: description}}
                }]}}}})
            }
            const oldTitle = "File:Old.png"
            const newTitle = "File:New.png"
            const oldUrl = "https://example.org/old.png"
            const newUrl = "https://example.org/new.png"
            networkFixtures.addFixture("GET", imageInfoUrl(oldTitle, true), details(oldTitle, oldUrl, "Old details"))
            networkFixtures.addFixture("GET", imageInfoUrl(newTitle, true), details(newTitle, newUrl, "New details"))

            networkFixtures.deferNextReply(true)
            ImageSelectionState.selectGalleryImage(["thumbnail-old.png", "thumbnail-new.png"],
                                                   ["Old caption", "New caption"],
                                                   [oldTitle, newTitle], 0)
            tryCompare(networkFixtures, "pendingReplyCount", 1)
            ImageSelectionState.selectNextImage()
            tryCompare(ImageSelectionState, "currentImageUrl", newUrl)
            compare(ImageSelectionState.currentImageDescription, "New details")

            networkFixtures.completeReply(imageInfoUrl(oldTitle, true))
            wait(50)
            compare(ImageSelectionState.currentImageUrl, newUrl)
            compare(ImageSelectionState.currentImageDescription, "New details")
        }

        function test_gallery_image_format_data() {
            return [
                { tag: "svg-preview", url: "https://example.org/logo.svg?tracking=1", mime: "image/svg+xml", preview: imageUrl, expected: imageUrl },
                { tag: "svg-suffix-without-mime", url: "https://example.org/logo.SVG?tracking=1", mime: "", preview: imageUrl, expected: imageUrl },
                { tag: "raster-original", url: imageUrl, mime: "image/png", preview: "https://example.org/small.png", expected: "https://example.org/small.png" },
                { tag: "svg-without-preview", url: "https://example.org/logo.svg", mime: "image/svg+xml", preview: "", expected: "https://example.org/logo.svg" }
            ]
        }

        function test_gallery_image_format(data) {
            const pageId = 302
            const title = "File:Logo.svg"
            var pages = {}
            pages[pageId] = { pageid: pageId, title: "Logo article", images: [{ title: title }] }
            networkFixtures.addFixture("GET", pageQueryUrl(pageId),
                                       JSON.stringify({ query: { pages: pages } }))
            networkFixtures.addFixture("GET", imageInfoUrl(title),
                                       JSON.stringify({ query: { pages: { "42": { imageinfo: [{
                                           url: data.url, mime: data.mime, thumburl: data.preview,
                                           extmetadata: { ImageDescription: { value: imageDescription } }
                                       }] } } } }))
            gallery = galleryModelComponent.createObject(root)
            gallery.loadImagesForPage(pageId)
            tryCompare(gallery, "isLoading", false)
            compare(gallery.imageUrls.length, 1)
            compare(gallery.imageUrls[0], data.expected)
            compare(gallery.imageDescriptions[0], imageDescription)
            compare(networkFixtures.requestCount, 2)
        }

        function test_selecting_gallery_image_shows_caption() {
            ArticleState.loadArticleByPageId(articleId)
            tryCompare(ArticleState, "isLoading", true)
            tryCompare(ArticleState, "isLoading", false)
            compare(ArticleState.currentPageId, articleId)
            compare(HistoryState.history.length, 1)
            compare(HistoryState.history[0].pageId, articleId)

            const originalHtml = ArticleState.currentPageExtract
            const originalHistory = HistoryState.history[0].timestamp
            gallery = galleryComponent.createObject(root)
            verify(gallery !== null, "ImageGallery must be created")
            // The gallery's metadata and thumbnail each load through the fake
            // transport; the fullscreen view requests the selected PNG too.
            tryVerify(function() { return networkFixtures.requestCount >= 5 })
            compare(apiRequestCount(), 4)

            compare(ArticleState.currentPageExtract, originalHtml)
            compare(ArticleState.errorMessage, "")
            compare(HistoryState.history.length, 1)
            compare(HistoryState.history[0].timestamp, originalHistory)
            const cell = child("imageGalleryCell-0")
            verify(waitForRendering(cell))
            const mainWindow = root.Window.window
            const originalVisibility = mainWindow.visibility
            mouseClick(cell)
            tryCompare(ImageSelectionState, "currentImageUrl", imageUrl + "#fullscreen")
            compare(ImageSelectionState.currentImageDescription, imageDescription)
            const viewer = child("imageFullscreenWindow")
            tryCompare(viewer, "visibility", Window.FullScreen)
            compare(viewer.transientParent, mainWindow)
            compare(mainWindow.visibility, originalVisibility)
            compare(stackView.depth, 0)

            const fullImage = child("fullScreenImage")
            tryCompare(fullImage, "status", Image.Ready)
            compare(fullImage.source.toString(), imageUrl + "#fullscreen")
            compare(ImageSelectionState.currentImageDescription, imageDescription)

            const captionOverlay = child("imageCaptionOverlay")
            tryCompare(captionOverlay, "visible", true)
            const caption = child("imageCaptionText")
            compare(caption.text, imageDescription)
            // Native fullscreen resize/configure events can reload the image
            // at a different sourceSize. API requests must still occur once.
            compare(apiRequestCount(), 5)
            verify(testSupport.requestCount >= 7)

            // The offscreen platform does not expose the secondary native
            // window for mouse synthesis; exercise the Close button's signal.
            child("imageBackButton").clicked()
            tryCompare(viewer, "visible", false)
            compare(gallery.visible, true)
            compare(mainWindow.visibility, originalVisibility)
            compare(stackView.depth, 0)

            // Escape must work in the viewer's native window, including when
            // a control within it owns keyboard focus.
            const refreshedCell = child("imageGalleryCell-0")
            verify(waitForRendering(refreshedCell))
            mouseClick(refreshedCell)
            tryCompare(viewer, "visibility", Window.FullScreen)
            tryCompare(viewer, "active", true)
            child("imageBackButton").forceActiveFocus()
            const escapeCount = root.underlyingEscapeCount
            verify(testSupport.sendKeyClick(viewer, Qt.Key_Escape))
            tryCompare(viewer, "visible", false)
            compare(root.underlyingEscapeCount, escapeCount)
            compare(mainWindow.visibility, originalVisibility)
            compare(stackView.depth, 0)

            // Mouse Back closes only the fullscreen window.
            mouseClick(refreshedCell)
            tryCompare(viewer, "visibility", Window.FullScreen)
            tryCompare(viewer, "active", true)
            verify(testSupport.sendBackButtonPress(viewer))
            let pressProcessed = false
            Qt.callLater(function() { pressProcessed = true })
            tryVerify(function() { return pressProcessed })
            compare(viewer.visible, true, "Keep the viewer open until the mouse grab is released")
            verify(testSupport.sendBackButtonRelease(viewer))
            tryCompare(viewer, "visible", false)
            compare(gallery.visible, true)
            compare(mainWindow.visibility, originalVisibility)
            compare(stackView.depth, 0)
            mouseMove(root, root.width - 1, root.height - 1)
            mouseMove(refreshedCell, refreshedCell.width / 2, refreshedCell.height / 2)
            tryCompare(child("imageGalleryMouseArea-0"), "containsMouse", true)

            mouseClick(refreshedCell)
            tryCompare(viewer, "visibility", Window.FullScreen)
            ImageSelectionState.selectImage("", "")
            tryCompare(viewer, "visible", false)

            const requestCount = apiRequestCount()
            ArticleState.loadArticleByPageId(articleId)
            compare(apiRequestCount(), requestCount, "Gallery must preserve the cached HTML article")
            compare(ArticleState.currentPageExtract, originalHtml)
        }

        function test_viewer_choice_survives_gallery_recreation() {
            ArticleState.loadArticleByPageId(articleId)
            tryCompare(ArticleState, "isLoading", false)
            gallery = galleryComponent.createObject(root)
            verify(gallery !== null)
            mouseClick(child("imageFullscreenSwitch"))
            compare(child("imageFullscreenSwitch").checked, false)
            gallery.destroy()
            gallery = null
            tryVerify(function() { return findChild(root, "imageFullscreenSwitch") === null })

            gallery = galleryComponent.createObject(root)
            verify(gallery !== null)
            compare(child("imageFullscreenSwitch").checked, false)
            mouseClick(child("imageFullscreenSwitch"))
            compare(child("imageFullscreenSwitch").checked, true)
        }

        function test_selecting_image_with_fullscreen_disabled() {
            ArticleState.loadArticleByPageId(articleId)
            tryCompare(ArticleState, "isLoading", false)
            gallery = galleryComponent.createObject(root)
            verify(gallery !== null)
            stackView.push(gallery, StackView.Immediate)
            tryVerify(function() { return findChild(root, "imageGalleryCell-0") !== null })
            const toggle = child("imageFullscreenSwitch")
            compare(toggle.checked, true)
            mouseClick(toggle)
            compare(toggle.checked, false)
            const cell = child("imageGalleryCell-0")
            verify(waitForRendering(cell))
            const mainWindow = root.Window.window
            const originalVisibility = mainWindow.visibility
            mouseClick(cell)
            tryCompare(stackView, "depth", 2)
            tryCompare(stackView, "busy", false)
            compare(child("imageFullscreenWindow").visible, false)
            compare(mainWindow.visibility, originalVisibility)
            tryCompare(ImageSelectionState, "currentImageUrl", imageUrl + "#fullscreen")
            const fullImage = child("fullScreenImage")
            tryCompare(fullImage, "status", Image.Ready)
            compare(child("imageCaptionText").text, imageDescription)

            mouseClick(child("imageBackButton"))
            tryCompare(stackView, "depth", 1)
            tryCompare(stackView, "busy", false)
            compare(stackView.currentItem, gallery)
            compare(toggle.checked, false)
            mouseClick(toggle)
            mouseClick(cell)
            tryCompare(child("imageFullscreenWindow"), "visibility", Window.FullScreen)
            compare(stackView.depth, 1)
            child("imageBackButton").clicked()
        }
    }
}
