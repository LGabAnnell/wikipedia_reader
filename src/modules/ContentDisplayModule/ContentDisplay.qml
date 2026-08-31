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

    signal backRequested

    property Timer searchDebounceTimer: Timer {
        interval: 200
        repeat: false
        onTriggered: contentDisplay.performSearch(searchField.text, articleSection.getText(0, articleSection.text.length))
    }

    height: parent ? parent.height : 0
    width: parent ? parent.width : 0

    onArticleTextChanged: {
        scrollView.ScrollBar.vertical.position = 0;
    }

    BusyIndicator {
        id: loadingIndicator

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

                    color: articleDisplay.sysPalette.text
                    font.pixelSize: 14
                    readOnly: true
                    selectByMouse: true
                    selectionColor: articleDisplay.sysPalette.highlight
                    text: mainContent.articleText
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
}
