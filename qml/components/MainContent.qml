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

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: !GlobalState.isLoading

        // Search results section
        ListView {
            id: searchResultsList
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: GlobalState.searchResults.length > 0
            model: GlobalState.searchResults

            // Custom delegate to show title and snippet
            delegate: Item {
                width: searchResultsList.width
                height: 100
                Column {
                    spacing: 5
                    padding: 10
                    width: parent.width

                    // Title
                    Label {
                        text: modelData.title
                        font.pixelSize: 16
                        font.bold: true
                        color: "#333"
                        elide: Text.ElideRight
                    }

                    // Snippet
                    Label {
                        text: modelData.snippet
                        font.pixelSize: 12
                        color: "#666"
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                    }

                    // Separator
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#eee"
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        // When a search result is clicked, fetch the full page
                        SearchBarModel.performSearch(modelData.title);
                    }
                    onEntered: parent.opacity = 0.8
                    onExited: parent.opacity = 1.0
                }
            }

            header: Label {
                text: "Search Results (" + GlobalState.searchResults.length + ")"
                font.pixelSize: 18
                padding: 10
                font.bold: true
                color: "#444"
            }

            // Add some spacing between items
            spacing: 5
        }

        // Page content section
        Frame {
            id: pageContentFrame
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: GlobalState.currentPageTitle.length > 0

            Column {
                spacing: 10
                padding: 10

                Label {
                    text: GlobalState.currentPageTitle
                    font.pixelSize: 24
                    font.bold: true
                    color: "#333"
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Text {
                        text: GlobalState.currentPageExtract
                        wrapMode: Text.WordWrap
                        font.pixelSize: 14
                        color: "#444"
                        padding: 5
                    }
                }

                // Back button to return to search results
                Button {
                    text: "Back to Search Results"
                    onClicked: {
                        GlobalState.setCurrentPage({
                            title: "",
                            extract: "",
                            pageid: -1
                        });
                    }
                    Layout.alignment: Qt.AlignRight
                    padding: 10
                }
            }
        }

        // Error message display
        Label {
            id: errorMessage
            text: GlobalState.errorMessage
            color: "red"
            font.pixelSize: 16
            padding: 20
            visible: GlobalState.errorMessage.length > 0
            Layout.alignment: Qt.AlignCenter
        }

        // Default welcome message
        Label {
            id: welcomeMessage
            text: "Welcome to Wikipedia Qt!\nEnter a search term above to get started."
            wrapMode: Text.Wrap
            font.pixelSize: 16
            padding: 20
            visible: GlobalState.searchResults.length === 0 && GlobalState.currentPageTitle.length === 0 && !errorMessage.visible
            Layout.alignment: Qt.AlignCenter
            color: "#666"
        }
    }
}
