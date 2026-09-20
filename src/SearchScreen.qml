// src/SearchScreen.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt 1.0
import wikipedia_qt.SearchBar 1.0
import wikipedia_qt.Sidebar 1.0

Item {
    id: searchScreen
    // Let the parent StackView manage our size
    ColumnLayout {
        // Use the searchScreen's dimensions
        spacing: 0

        anchors.fill: parent

        SearchBar {
            id: searchBar
            Layout.fillWidth: true
        }

        BusyIndicator {
            objectName: "searchLoadingIndicator"
            running: searchBar.isSearching
            visible: running
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            objectName: "searchStatusLabel"
            text: GlobalState.errorMessage.length > 0
                  ? GlobalState.errorMessage
                  : (searchBar.hasCompletedSearch && GlobalState.searchResults.length === 0
                     ? qsTr("No results found.") : "")
            visible: text.length > 0 && !searchBar.isSearching
            Layout.alignment: Qt.AlignHCenter
        }

        SplitView {
            Layout.fillHeight: true
            Layout.fillWidth: true
            orientation: Qt.Horizontal

            Sidebar {
                id: sidebar
                objectName: "searchResults"
                SplitView.fillHeight: true
                SplitView.minimumWidth: 200
                searchResults: GlobalState.searchResults ? GlobalState.searchResults : []
            }
        }
    }
}
