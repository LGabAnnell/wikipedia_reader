// src/SearchScreen.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt 1.0
import wikipedia_qt.SearchBar 1.0
import wikipedia_qt.Sidebar 1.0

Item {
    id: searchScreen
    Layout.fillWidth: true
    Layout.fillHeight: true

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        SearchBar {}

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

            // Placeholder for empty space
            Item {
                SplitView.fillHeight: true
                SplitView.minimumWidth: 200
            }
        }
    }
}