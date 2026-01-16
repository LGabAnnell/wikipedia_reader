// Main.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt 1.0
import SearchBar 1.0
import Sidebar 1.0

ApplicationWindow {
    id: root
    visible: true
    width: 800
    height: 600
    title: "Browse Wikipedia"
    Layout.fillWidth: true
    Layout.fillHeight: true

    header: Header {
        Layout.fillWidth: true
    }

    ColumnLayout {
        width: parent.width
        height: parent.height
        spacing: 0

        SearchBar {
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            Sidebar {
                Layout.preferredWidth: 200
                Layout.minimumWidth: 40
                Layout.maximumWidth: 200
            }

            MainContent {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
}
