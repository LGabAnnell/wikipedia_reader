// Main.qml
import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt 1.0

ApplicationWindow {
    // id: root
    visible: true
    width: 800
    height: 600
    title: "Browsen Wikipedia"

    header: Header {}

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        SearchBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            Layout.rightMargin: 10
            Layout.leftMargin: 10
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Sidebar {}

            MainContent {}
        }
    }
}
