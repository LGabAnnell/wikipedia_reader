import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt 1.0

RowLayout {
    id: searchBar
    Layout.fillWidth: true

    TextField {
        id: searchText
        Layout.fillWidth: true
        placeholderText: "Search Wikipedia"
        onTextChanged: SearchBarModel.searchText = text
        enabled: !SearchBarModel.isSearching
        font.pixelSize: 16
        color: "#888"
    }

    Button {
        id: searchButton
        text: "Search"
        onClicked: SearchBarModel.performSearch()
    }
}
