// Main.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt 1.0
import SearchBar 1.0

ApplicationWindow {
    // id: root
    visible: true
    width: 800
    height: 600
    title: "Browse Wikipedia"

    header: Header {}

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        Layout.fillWidth: true
        Layout.fillHeight: true

        SearchBar {}

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Sidebar {}

            MainContent {}
        }
    }
}
