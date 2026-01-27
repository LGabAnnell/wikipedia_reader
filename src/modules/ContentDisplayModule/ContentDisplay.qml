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
        sequences: [StandardKey.Find]
        onActivated: {
            searchBar.visible = true;
            searchField.forceActiveFocus();
        }
    }

    Shortcut {
        sequences: ["Escape"]
        onActivated: {
            searchBar.visible = false;
        }
    }

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        spacing: 0

        RowLayout {
            id: searchBar
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            spacing: 10
            z: 1
            visible: false

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: "Search..."
                z: 1
                onTextChanged: function () {
                    contentDisplay.searchRequested(text, articleSection.getText(0, articleSection.text.length));
                }

                Keys.onReturnPressed: function (event) {
                    if (event.modifiers & Qt.ShiftModifier) {
                        contentDisplay.navigateToPreviousResult();
                        return;
                    }
                    
                    contentDisplay.navigateToNextResult();
                }
            }

            Text {
                text: contentDisplay.totalResults > 0 ? contentDisplay.currentResultIndex + " of " + contentDisplay.totalResults : ""
                color: "white"
                font.pixelSize: 14
                verticalAlignment: Text.AlignVCenter
            }

            Button {
                id: upButton
                text: "↑"
                onClicked: {
                    contentDisplay.navigateToPreviousResult();
                }
            }

            Button {
                id: downButton
                text: "↓"
                onClicked: {
                    contentDisplay.navigateToNextResult();
                }
            }
        }

        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
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
    }

    ContentDisplayModel {
        id: contentDisplay
        onSearchResultsAvailable: function (indices) {
            if (indices.length === 0) {
                articleSection.select(0, 0);
                return;
            }

            articleSection.cursorPosition = indices[0].start;
            articleSection.select(indices[0].start, indices[0].end);

            // Manually set the contentY property of the Flickable
            scrollView.scrollToCursor();
        }

        onNavigateToResult: function (start, end) {
            articleSection.cursorPosition = start;
            articleSection.select(start, end);
            scrollView.scrollToCursor();
        }
    }
}
