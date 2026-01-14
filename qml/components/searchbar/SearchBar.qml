import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt 1.0

RowLayout {
    id: searchBar
    Layout.fillWidth: true
    spacing: 10
    // Remove padding property as RowLayout doesn't support it

    // Add margin using anchors instead
    anchors.left: parent.left
    anchors.leftMargin: 10
    anchors.right: parent.right
    anchors.rightMargin: 10
    anchors.top: parent.top
    anchors.topMargin: 10
    TextField {
        id: searchText
        Layout.fillWidth: true
        placeholderText: "Search Wikipedia"
        onTextChanged: SearchBarModel.searchText = text
        enabled: !SearchBarModel.isSearching
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
        enabled: !SearchBarModel.isSearching && searchText.text.length > 0
        onClicked: SearchBarModel.performSearch()

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
