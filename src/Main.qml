// Main.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt 1.0
import wikipedia_qt.SearchBar 1.0
import wikipedia_qt.Sidebar 1.0

ApplicationWindow {
    id: root
    visible: true
    width: 800
    height: 600
    title: "Browse Wikipedia"

    header: Header {
        Layout.fillWidth: true
    }

    ColumnLayout {
        width: parent.width
        height: parent.height
        spacing: 0

        SearchBar {}

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
            SplitView {
                height: parent.height
                width: parent.width
                orientation: Qt.Horizontal

                Sidebar {
                    id: sidebar
                    SplitView.fillHeight: true
                    SplitView.minimumWidth: 200
                    searchResults: GlobalState.searchResults ? GlobalState.searchResults : []
                }

                MainContent {
                    id: mainContent
                    SplitView.minimumWidth: 200
                    SplitView.fillHeight: true
                }
            }
        }
    }
}
