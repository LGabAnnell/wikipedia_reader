// Sidebar.qml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import wikipedia_qt 1.0

Item {
    id: container
    clip: true
    height: 200
    property list<search_result> searchResults

    ListView {
        id: listView
        anchors.fill: parent
        model: container.searchResults
        delegate: ItemDelegate {
            id: delegate
            focus: false
            SystemPalette {
                id: sysPalette
            }

            hoverEnabled: true
            width: parent ? parent.width : 0
            height: 80
            padding: 10
            clip: true
            focusPolicy: Qt.NoFocus

            background: Rectangle {
                id: bgRect
                color: sysPalette.button
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
                    if (listView.currentIndex == index) {
                        return;
                    }
                    parent.background.color = sysPalette.highlight;
                    bgRect.color.a = .5;
                    bgRect.border.color = sysPalette.highlight;
                    bgRect.border.width = 2;
                }
                onExited: function () {
                    parent.background.color = sysPalette.button;
                    bgRect.border.width = 0;
                }
                onPressed: function () {
                    listView.currentIndex = index;
                }
                onClicked: function () {
                    if (modelData.pageid > 0) {
                        GlobalState.loadArticleByPageId(modelData.pageid);
                        NavigationState.navigateToContent();
                    }
                }
            }
        }

        spacing: 10
    }
}

