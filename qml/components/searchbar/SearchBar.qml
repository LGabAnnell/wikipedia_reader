import QtQuick
import QtQuick.Controls

Item {
    id: searchBar
    width: parent.width

    TextField {
        id: searchText
        width: parent.width
        placeholderText: "Search Wikipedia"
        anchors.centerIn: parent
        font.pixelSize: 16
        color: "#888"
    }
}
