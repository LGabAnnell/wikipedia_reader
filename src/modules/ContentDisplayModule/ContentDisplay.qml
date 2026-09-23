// ContentDisplay.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt
import wikipedia_qt.ContentDisplay 1.0
import wikipedia_qt.Section 1.0

Item {
    id: mainContent

    property string articleText: ""
    property bool sectionResizeInProgress: false

    signal backRequested

    function decodeHtmlAttribute(value) {
        return value.replace(/&amp;/g, "&")
                    .replace(/&quot;/g, '"')
                    .replace(/&#39;/g, "'")
                    .replace(/&lt;/g, "<")
                    .replace(/&gt;/g, ">");
    }

    function fullResolutionImageUrl(imageUrl) {
        const uploadHost = "https://upload.wikimedia.org/";
        const thumbHost = "https://thumb.wikimedia.org/";
        const isUploadThumb = imageUrl.startsWith(uploadHost);
        const isThumbHost = imageUrl.startsWith(thumbHost);
        const thumbnailMarker = "/thumb/";
        const markerPosition = imageUrl.indexOf(thumbnailMarker);
        if (markerPosition < 0 || (!isUploadThumb && !isThumbHost))
            return imageUrl;

        // A thumbnail URL is <host>/<bucket>/thumb/<hash>/<file>/<size-segment>;
        // the original asset is upload.wikimedia.org/<bucket>/<hash>/<file>.
        // Query and fragment (tracking parameters) do not apply to the original.
        let pathEnd = imageUrl.length;
        const queryPosition = imageUrl.indexOf("?", markerPosition);
        const fragmentPosition = imageUrl.indexOf("#", markerPosition);
        if (queryPosition >= 0)
            pathEnd = Math.min(pathEnd, queryPosition);
        if (fragmentPosition >= 0)
            pathEnd = Math.min(pathEnd, fragmentPosition);

        const hostLength = isThumbHost ? thumbHost.length : uploadHost.length;
        const bucketPath = imageUrl.substring(hostLength, markerPosition);
        const thumbnailPath = imageUrl.substring(markerPosition + thumbnailMarker.length,
                                                 pathEnd);
        const sizeSegmentPosition = thumbnailPath.lastIndexOf("/");
        if (sizeSegmentPosition < 0)
            return imageUrl;

        return uploadHost + bucketPath + "/"
                + thumbnailPath.substring(0, sizeSegmentPosition);
    }

    function renderArticleText(html) {
        return html.replace(/<a\b[^>]*>[\s\S]*?<\/a\s*>/gi, function (anchor) {
            const imageMatch = anchor.match(/<img\b[^>]*>/i);
            if (!imageMatch)
                return anchor;

            const imageTag = imageMatch[0];
            const srcMatch = imageTag.match(/\bsrc\s*=\s*(["'])(.*?)\1/i);
            if (!srcMatch)
                return anchor;

            const altMatch = imageTag.match(/\b(?:alt|title)\s*=\s*(["'])(.*?)\1/i);
            let imageUrl = decodeHtmlAttribute(srcMatch[2]);
            if (imageUrl.startsWith("//"))
                imageUrl = "https:" + imageUrl;
            imageUrl = fullResolutionImageUrl(imageUrl);

            const imageData = encodeURIComponent(JSON.stringify({
                url: imageUrl,
                description: altMatch ? decodeHtmlAttribute(altMatch[2]) : ""
            }));
            const imageLink = "wikipedia-image:" + imageData;
            return anchor.replace(/(\bhref\s*=\s*)(["'])(.*?)\2/i,
                                 function (_, prefix, quote) {
                                     return prefix + quote + imageLink + quote;
                                 });
        });
    }

    property Timer searchDebounceTimer: Timer {
        interval: 200
        repeat: false
        onTriggered: contentDisplay.performSearch(searchField.text, articleSection.getText(0, articleSection.text.length))
    }

    // Debounces the scroll-tracking lookup so rapid contentY changes don't
    // flood positionAt() calls during momentum scrolling.
    Timer {
        id: sectionTrackTimer
        interval: 50
        repeat: false
        onTriggered: {
            if (mainContent.sectionResizeInProgress
                    || !articleSection.visible || sectionBar.sections.length === 0)
                return;

            var contentY = scrollView.contentItem.contentY;
            var point = articleSection.mapFromItem(articleDisplay, 0, contentY);
            var charPos = articleSection.positionAt(point.x, point.y);
            var sectionIdx = contentDisplay.findSectionAtPosition(charPos);
            GlobalState.setCurrentSectionIndex(sectionIdx);
        }
    }

    // A Behavior owns the section-width animation, so its child animation is
    // not a dependable place to observe completion. Restarting this inexpensive
    // timer as the width changes lets us restore the pinned position once the
    // layout has settled without doing document lookups on every frame.
    Timer {
        id: sectionResizeSettleTimer
        interval: 50
        repeat: false
        onTriggered: {
            scrollView.applyPinnedPosition();
            scrollView.pinnedPosition = -1;
            mainContent.sectionResizeInProgress = false;
            sectionTrackTimer.restart();
        }
    }

    // (Re)compute the character-position map for all sections. Called when
    // both the article HTML and the section list are available.
    function updateSectionTracking() {
        if (articleText.length > 0 && sectionBar.sections.length > 0) {
            contentDisplay.updateSectionPositions(articleText, sectionBar.sections);
        }
    }

    height: parent ? parent.height : 0
    width: parent ? parent.width : 0

    onArticleTextChanged: {
        scrollView.ScrollBar.vertical.position = 0;
        GlobalState.setCurrentSectionIndex(-1);
        updateSectionTracking();
    }

    BusyIndicator {
        id: loadingIndicator
        objectName: "articleLoadingIndicator"

        anchors.centerIn: parent
        running: GlobalState.isLoading
        visible: GlobalState.isLoading
    }
    Shortcut {
        sequences: [StandardKey.Find]

        onActivated: {
            searchBar.visible = true;
            searchField.forceActiveFocus();
        }
    }
    RowLayout {
        id: contentRow

        anchors.fill: parent
        spacing: 0

        ColumnLayout {
            id: mainLayout

            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 0

            RowLayout {
                id: searchBar

                Layout.fillWidth: true
                Layout.leftMargin: 10
                Layout.preferredHeight: 60
                Layout.rightMargin: 10
                spacing: 10
                visible: false
                z: 1

            TextField {
                id: searchField

                Layout.fillWidth: true
                placeholderText: "Search..."
                z: 1

                Keys.onReturnPressed: function (event) {
                    if (event.modifiers & Qt.ShiftModifier) {
                        contentDisplay.navigateToPreviousResult();
                        return;
                    }

                    contentDisplay.navigateToNextResult();
                }
                onTextChanged: function () {
                    searchDebounceTimer.restart();
                }
            }
            Text {
                color: articleDisplay.sysPalette.text
                font.pixelSize: 14
                text: contentDisplay.totalResults > 0 ? contentDisplay.currentResultIndex + " of " + contentDisplay.totalResults : ""
                verticalAlignment: Text.AlignVCenter
            }
            Button {
                id: upButton

                text: "↑"
                Accessible.name: "Previous search result"

                onClicked: {
                    contentDisplay.navigateToPreviousResult();
                }
            }
            Button {
                id: downButton

                text: "↓"
                Accessible.name: "Next search result"

                onClicked: {
                    contentDisplay.navigateToNextResult();
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 10
            Layout.preferredHeight: 40
            Layout.rightMargin: 10
            spacing: 10

            Item { Layout.fillWidth: true }
            Button {
                id: galleryButton

                text: "Gallery"
                visible: GlobalState.currentPageTitle.length > 0

                onClicked: {
                    NavigationState.navigateToImageGallery();
                }
            }
            Button {
                id: copyHtmlButton

                text: "Copy HTML"
                visible: GlobalState.currentPageTitle.length > 0 && mainContent.articleText.length > 0

                onClicked: {
                    GlobalState.copyToClipboard(mainContent.articleText);
                }
            }
        }
        ScrollView {
            id: scrollView

            // Character position in articleSection that should stay pinned to the
            // top of the viewport while the sidebar toggles and reflows the text.
            property int pinnedPosition: -1
            property real pinnedOffset: 0

            function pinTopPosition() {
                let contentY = scrollView.contentItem.contentY;
                let point = articleSection.mapFromItem(articleDisplay, 0, contentY);
                scrollView.pinnedPosition = articleSection.positionAt(point.x, point.y);
                let rect = articleSection.positionToRectangle(scrollView.pinnedPosition);
                let charGlobalY = articleSection.mapToItem(articleDisplay, 0, rect.y).y;
                scrollView.pinnedOffset = contentY - charGlobalY;
            }

            function applyPinnedPosition() {
                if (scrollView.pinnedPosition < 0)
                    return;
                let rect = articleSection.positionToRectangle(scrollView.pinnedPosition);
                let charGlobalY = articleSection.mapToItem(articleDisplay, 0, rect.y).y;
                let newContentY = charGlobalY + scrollView.pinnedOffset;
                let maxContentY = Math.max(0, scrollView.contentHeight - scrollView.height);
                scrollView.contentItem.contentY = Math.max(0, Math.min(newContentY, maxContentY));
            }

            function scrollToCursor(offset) {
                const cursorRect = articleSection.cursorRectangle;
                // Map into articleDisplay (the Column), NOT scrollView.contentItem —
                // articleDisplay scrolls together with articleSection, so this gives
                // an absolute, scroll-independent position within the content.
                const contentPoint = articleSection.mapToItem(articleDisplay, 0, cursorRect.y);

                let scrollToY = contentPoint.y - (offset || cursorRect.height);
                scrollToY = Math.max(0, Math.min(scrollToY, scrollView.contentHeight - scrollView.height));

                scrollView.contentItem.contentY = scrollToY;
            }

            Layout.fillHeight: true
            Layout.fillWidth: true
            ScrollBar.vertical.interactive: true
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn
            clip: true

            Column {
                id: articleDisplay

                property SystemPalette sysPalette: SystemPalette {
                }

                padding: 20
                spacing: 20
                width: scrollView.width - scrollView.effectiveScrollBarWidth - leftPadding - rightPadding

                TextEdit {
                    objectName: "articleTitle"
                    color: articleDisplay.sysPalette.text
                    font.bold: true
                    font.pixelSize: 20
                    readOnly: true
                    selectByMouse: true
                    selectionColor: articleDisplay.sysPalette.highlight
                    text: GlobalState.currentPageTitle
                    visible: GlobalState.currentPageTitle.length > 0
                    width: parent.width
                    wrapMode: TextEdit.Wrap
                }
                TextEdit {
                    id: articleSection
                    objectName: "articleBody"

                    color: articleDisplay.sysPalette.text
                    font.pixelSize: 14
                    readOnly: true
                    selectByMouse: true
                    selectionColor: articleDisplay.sysPalette.highlight
                    text: mainContent.renderArticleText(mainContent.articleText)
                    textFormat: TextEdit.RichText
                    visible: mainContent.articleText.length > 0
                    width: parent.width
                    wrapMode: TextEdit.Wrap

                    ContextMenu.menu: Menu {
                        MenuItem {
                            text: "Copy"

                            onTriggered: articleSection.copy()
                        }
                    }

                    onLinkActivated: function (link) {
                        const imageLinkPrefix = "wikipedia-image:";
                        if (link.startsWith(imageLinkPrefix)) {
                            let image = null;
                            try {
                                const imageData = decodeURIComponent(link.substring(imageLinkPrefix.length));
                                image = JSON.parse(imageData);
                            } catch (error) {
                                console.warn("Unable to read inline image link", error);
                            }
                            if (!image || !image.url)
                                return;

                            GlobalState.currentImageUrl = image.url;
                            GlobalState.currentImageDescription = image.description;
                            NavigationState.navigateToView(Constants.imageView);
                            return;
                        }

                        if (link.startsWith("/wiki/")) {
                            var title = link.substring(6).replace(/_/g, " ");
                            GlobalState.loadArticleByTitle(title);
                        } else {
                            Qt.openUrlExternally(link);
                        }
                    }
                    onLinkHovered: {}
                }
                Text {
                    color: articleDisplay.sysPalette.text
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    text: "Select an article to view its content"
                    visible: GlobalState.currentPageTitle.length === 0 && !GlobalState.isLoading
                    width: parent.width
                    wrapMode: Text.WordWrap
                }
                Text {
                    objectName: "articleErrorMessage"
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: articleDisplay.sysPalette.negativeText || articleDisplay.sysPalette.text
                    text: GlobalState.errorMessage
                    visible: GlobalState.errorMessage.length > 0
                    width: parent.width
                    wrapMode: Text.WordWrap
                }
            }
        }
        }

        // Permanent collapsible sections bar anchored to the right
        Section {
            id: sectionBar

            Layout.alignment: Qt.AlignRight
            Layout.fillHeight: true
            Layout.preferredWidth: collapsed ? collapsedWidth : expandedWidth
            visible: GlobalState.currentPageTitle.length > 0

            onResizeStarted: {
                if (!mainContent.sectionResizeInProgress)
                    scrollView.pinTopPosition();
                mainContent.sectionResizeInProgress = true;
                sectionTrackTimer.stop();
                sectionResizeSettleTimer.restart();
            }
            onWidthChanged: {
                if (mainContent.sectionResizeInProgress)
                    sectionResizeSettleTimer.restart();
            }
            onSectionClicked: function (section) {
                var html = articleSection.text;
                var cursorPos = contentDisplay.findSectionPosition(html, section.anchor);

                if (cursorPos !== -1) {
                    articleSection.cursorPosition = cursorPos;
                    articleSection.select(cursorPos, cursorPos);
                    scrollView.scrollToCursor();
                }
            }
        }
    }
    ContentDisplayModel {
        id: contentDisplay

        onNavigateToResult: function (start, end) {
            articleSection.cursorPosition = start;
            articleSection.select(start, end);
            scrollView.scrollToCursor(scrollView.height / 2);
        }
        onSearchResultsAvailable: function (indices) {
            if (indices.length === 0) {
                articleSection.select(0, 0);
                return;
            }

            articleSection.cursorPosition = indices[0].start;
            articleSection.select(indices[0].start, indices[0].end);

            // Manually set the contentY property of the Flickable
            scrollView.scrollToCursor(scrollView.height / 2);
        }
    }

    // Close the search bar on escape key when visible
    Shortcut {
        sequences: ["Escape"]

        onActivated: {
            if (searchBar.visible) {
                searchBar.visible = false;
            }
        }
    }

    // Track which section is at the top of the viewport as the user scrolls.
    Connections {
        target: scrollView.contentItem
        function onContentYChanged() {
            if (!mainContent.sectionResizeInProgress)
                sectionTrackTimer.restart();
        }
    }

    // Re-compute section positions when the section list arrives asynchronously.
    Connections {
        target: sectionBar
        function onSectionsChanged() {
            updateSectionTracking();
        }
    }

    // Reset the highlight when navigating to a different article.
    Connections {
        target: GlobalState
        function onCurrentPageChanged() {
            GlobalState.setCurrentSectionIndex(-1);
        }
    }
}
