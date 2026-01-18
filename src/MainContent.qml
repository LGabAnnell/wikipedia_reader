// MainContent.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt 1.0

Frame {
    Layout.fillWidth: true
    Layout.fillHeight: true
    padding: 0

    // Loading indicator
    BusyIndicator {
        id: loadingIndicator
        anchors.centerIn: parent
        running: GlobalState.isLoading
        visible: GlobalState.isLoading
    }

    // Display the article content
    Column {
        spacing: 10
        anchors.fill: parent
        anchors.margins: 10

        // Article title
        Text {
            text: GlobalState.currentPageTitle
            font.pixelSize: 20
            font.bold: true
            visible: GlobalState.currentPageTitle.length > 0
        }

        // Article content
        Text {
            text: GlobalState.currentPageExtract
            wrapMode: Text.WordWrap
            font.pixelSize: 14
            visible: GlobalState.currentPageExtract.length > 0
        }

        // Placeholder when no article is selected
        Text {
            text: "Select an article to view its content"
            wrapMode: Text.WordWrap
            font.pixelSize: 14
            anchors.centerIn: parent
            visible: GlobalState.currentPageTitle.length === 0 && !GlobalState.isLoading
        }

        // Error message display
        Text {
            text: GlobalState.errorMessage
            color: "red"
            visible: GlobalState.errorMessage.length > 0
            anchors.centerIn: parent
        }
    }
}
