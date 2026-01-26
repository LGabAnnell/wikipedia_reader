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
            Layout.fillWidth: true
        }

        SplitView {
            Layout.fillHeight: true
            Layout.fillWidth: true
            orientation: Qt.Horizontal

            Sidebar {
                id: sidebar
                SplitView.fillHeight: true
                SplitView.minimumWidth: 200
                searchResults: GlobalState.searchResults ? GlobalState.searchResults : []
            }
        }
    }
}