// MainContent.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt 1.0

Item {
    id: mainContent
    // Let the parent StackView manage our size
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    // Signal to notify parent components about back navigation
    signal backRequested

    // Loading indicator
    BusyIndicator {
        id: loadingIndicator
        anchors.centerIn: parent
        running: GlobalState.isLoading
        visible: GlobalState.isLoading
    }

    ScrollView {
        id: scrollView
        // Use the mainContent's dimensions
        width: mainContent.width
        height: mainContent.height
        clip: true

        // Add a vertical scrollbar
        ScrollBar.vertical.interactive: true
        ScrollBar.vertical.policy: ScrollBar.AlwaysOn

        Column {
            id: articleDisplay
            spacing: 20
            padding: 20
            width: scrollView.width - scrollView.effectiveScrollBarWidth - leftPadding - rightPadding

            property SystemPalette sysPalette: SystemPalette {}

            // Article title (using TextEdit for selectable text)
            TextEdit {
                text: GlobalState.currentPageTitle
                font.pixelSize: 20
                font.bold: true
                color: articleDisplay.sysPalette.text
                selectionColor: articleDisplay.sysPalette.highlight
                visible: GlobalState.currentPageTitle.length > 0
                width: parent.width
                wrapMode: TextEdit.Wrap
                readOnly: true // Make it read-only to prevent editing
                selectByMouse: true // Enable text selection
            }

            // Article content (using TextEdit for selectable text)
            TextEdit {
                id: articleSection
                text: GlobalState.currentPageExtract
                selectionColor: articleDisplay.sysPalette.highlight
                wrapMode: TextEdit.Wrap
                font.pixelSize: 14
                color: articleDisplay.sysPalette.text
                visible: GlobalState.currentPageExtract.length > 0
                width: parent.width
                readOnly: true // Make it read-only to prevent editing
                selectByMouse: true // Enable text selection
                ContextMenu.menu: Menu {
                    MenuItem {
                        text: "Copy"
                        onTriggered: function () {
                            articleSection.copy();
                        }
                    }
                }
            }

            // Placeholder when no article is selected
            Text {
                text: "Select an article to view its content"
                wrapMode: Text.WordWrap
                color: articleDisplay.sysPalette.text
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                visible: GlobalState.currentPageTitle.length === 0 && !GlobalState.isLoading
                width: parent.width
            }

            // Error message display
            Text {
                text: GlobalState.errorMessage
                color: "red"
                visible: GlobalState.errorMessage.length > 0
                width: parent.width
                wrapMode: Text.WordWrap
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
}
