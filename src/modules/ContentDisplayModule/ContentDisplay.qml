// ContentDisplay.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt 1.0
import wikipedia_qt.ContentDisplay 1.0

Item {
    id: mainContent
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    signal backRequested
    property string articleText: ""

    BusyIndicator {
        id: loadingIndicator
        anchors.centerIn: parent
        running: GlobalState.isLoading
        visible: GlobalState.isLoading
    }

    Shortcut {
        sequence: StandardKey.Find
        onActivated: {
            searchField.visible = !searchField.visible;
        }
    }

    TextField {
        id: searchField
        width: parent.width - 40
        placeholderText: "Search..."
        height: 40
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 20
        z: 1
        visible: false
        onTextChanged: function () {
            contentDisplay.searchRequested(text, articleSection.getText(0, articleSection.text.length));
        }
    }

    ScrollView {
        id: scrollView
        width: mainContent.width
        height: mainContent.height
        clip: true

        ScrollBar.vertical.interactive: true
        ScrollBar.vertical.policy: ScrollBar.AlwaysOn

        Column {
            id: articleDisplay
            spacing: 20
            padding: 20
            width: scrollView.width - scrollView.effectiveScrollBarWidth - leftPadding - rightPadding

            property SystemPalette sysPalette: SystemPalette {}

            TextEdit {
                text: GlobalState.currentPageTitle
                font.pixelSize: 20
                font.bold: true
                color: articleDisplay.sysPalette.text
                selectionColor: articleDisplay.sysPalette.highlight
                visible: GlobalState.currentPageTitle.length > 0
                width: parent.width
                wrapMode: TextEdit.Wrap
                readOnly: true
                selectByMouse: true
            }

            TextEdit {
                id: articleSection
                text: mainContent.articleText
                selectionColor: articleDisplay.sysPalette.highlight
                wrapMode: TextEdit.Wrap
                font.pixelSize: 14
                textFormat: TextEdit.RichText
                color: articleDisplay.sysPalette.text
                visible: mainContent.articleText.length > 0
                width: parent.width
                readOnly: true
                selectByMouse: true
                ContextMenu.menu: Menu {
                    MenuItem {
                        text: "Copy"
                        onTriggered: articleSection.copy()
                    }
                }

                onLinkHovered: {}
                onLinkActivated: function (link) {
                    console.warn("Clicked on link" + link);
                }
            }

            Text {
                text: "Select an article to view its content"
                wrapMode: Text.WordWrap
                color: articleDisplay.sysPalette.text
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                visible: GlobalState.currentPageTitle.length === 0 && !GlobalState.isLoading
                width: parent.width
            }

            Text {
                text: GlobalState.errorMessage
                color: "red"
                visible: GlobalState.errorMessage.length > 0
                width: parent.width
                wrapMode: Text.WordWrap
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        function scrollToCursor() {
            // Get the cursor rectangle in the TextEdit's coordinate system
            var cursorRect = articleSection.cursorRectangle;
            // Calculate the position to scroll to
            var scrollToY = cursorRect.y - scrollView.height / 2;
            // Ensure the position is within valid bounds
            scrollToY = Math.max(0, Math.min(scrollToY, scrollView.contentHeight - scrollView.height));
            // Use the ScrollBar's value property to set the scroll position
            scrollView.ScrollBar.vertical.position = scrollToY / (scrollView.contentHeight - scrollView.height);
        }
    }

    ContentDisplayModel {
        id: contentDisplay
        onSearchResultsAvailable: function (indices) {
            console.warn("Search results available: " + JSON.stringify(indices));
            articleSection.cursorPosition = indices[0].start;
            articleSection.select(indices[0].start, indices[0].end);

            // Manually set the contentY property of the Flickable
            scrollView.scrollToCursor();
        }
    }
}
