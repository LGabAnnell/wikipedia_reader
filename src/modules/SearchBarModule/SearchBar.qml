import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt.SearchBar 1.0

RowLayout {
    id: searchBar
    Layout.margins: 10
    spacing: 10

    SearchBarModel {
        id: searchBarModel
    }

    TextField {
        id: searchText
        placeholderText: "Search Wikipedia"
        onTextChanged: searchBarModel.searchText = text
        enabled: !searchBarModel.isSearching
        font.pixelSize: 16

        // Clear button
        rightPadding: 30
    }

    Button {
        id: searchButton
        text: "Search"
        enabled: !searchBarModel.isSearching && searchText.text.length > 0
        onClicked: searchBarModel.performSearch()

        Layout.preferredWidth: 100
    }
}
