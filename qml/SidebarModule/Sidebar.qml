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

            background: Rectangle {
                id: bgRect
                color: sysPalette.button
                radius: 5
            }

            Rectangle {
                visible: listView.currentIndex == index
                anchors.fill: parent
                color: sysPalette.highlight
                border.color: sysPalette.highlight
                border.width: 2
            }

            Column {
                Text {
                    text: modelData.title
                }
                Text {
                    text: modelData.snippet
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
