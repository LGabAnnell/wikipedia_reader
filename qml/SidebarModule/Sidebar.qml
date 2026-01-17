// Sidebar.qml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import wikipedia_qt 1.0

Item {
    clip: true
    width: 180
    height: 200
    property list<SearchResult> searchResults

    SystemPalette {
        id: sysPalette
    }

    ListView {
        id: listView
        anchors.fill: parent
        model: searchResults
        delegate: ItemDelegate {
            id: delegate
            hoverEnabled: true
            width: parent ? parent.width : 0
            height: 80
            padding: 10
            clip: true

            background: Rectangle {
                id: bgRect
                color: sysPalette.button
                radius: 5
            }

            Rectangle {
                id: selectedRect
                anchors.fill: parent
                anchors.margins: 5
                visible: listView.currentIndex == index
                color: sysPalette.highlight
                border.color: sysPalette.highlight
                border.width: 2
                radius: 5
            }

            Column {
                padding: 10
                spacing: 5
                anchors.fill: parent
                anchors.margins: 5
                clip: true
                Text {
                    text: modelData.title
                    color: sysPalette.text
                }
                Text {
                    text: modelData.snippet
                    color: sysPalette.text
                }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onEntered: function () {
                    parent.background.color = sysPalette.highlight
                    bgRect.color.a = .5
                    bgRect.border.color = sysPalette.highlight
                    bgRect.border.width = 2
                }
                onExited: function () {
                    parent.background.color = sysPalette.button
                    bgRect.border.width = 0
                }
                onPressed: function() {
                    listView.currentIndex = index
                }
            }
        }

        spacing: 10
    }
}

