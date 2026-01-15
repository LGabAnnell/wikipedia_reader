import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SearchBar 1.0

RowLayout {
    id: searchBar
    Layout.fillWidth: true
    spacing: 10
    // Remove padding property as RowLayout doesn't support it

    SearchBarModel {
        id: searchBarModel
    }

    TextField {
        id: searchText
        Layout.fillWidth: true
        placeholderText: "Search Wikipedia"
        onTextChanged: searchBarModel.searchText = text
        enabled: !searchBarModel.isSearching
        font.pixelSize: 16
        color: "#333"

        // Clear button
        rightPadding: 20

        Rectangle {
            anchors.right: searchText.right
            anchors.rightMargin: 5
            anchors.verticalCenter: searchText.verticalCenter
            width: 20
            height: 20
            color: "transparent"

            Image {
                anchors.centerIn: parent
                source: "qrc:/icons/safes.jpg"
                width: 16
                height: 16
                visible: searchText.text.length > 0

                MouseArea {
                    anchors.fill: parent
                    onClicked: searchText.text = ""
                }
            }
        }
    }

    Button {
        id: searchButton
        text: "Search"
        enabled: !searchBarModel.isSearching && searchText.text.length > 0
        onClicked: searchBarModel.performSearch()

        background: Rectangle {
            color: enabled ? "#4a90e2" : "#cccccc"
            radius: 4
        }

        contentItem: Text {
            text: searchButton.text
            color: "white"
            font.pixelSize: 14
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        padding: 10
    }
}
