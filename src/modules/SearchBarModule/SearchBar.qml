import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt.SearchBar 1.0

RowLayout {
    id: searchBar
    objectName: "searchBar"
    readonly property alias isSearching: searchBarModel.isSearching
    readonly property alias hasCompletedSearch: searchBarModel.hasCompletedSearch
    Layout.margins: 10
    spacing: 10

    SearchBarModel {
        id: searchBarModel
    }

    TextField {
        id: searchText
        objectName: "searchInput"
        placeholderText: "Search Wikipedia"
        onTextChanged: searchBarModel.searchText = text
        onAccepted: searchBarModel.performSearch()
        enabled: !searchBarModel.isSearching
        font.pixelSize: 16
        Layout.fillWidth: true
        // Clear button
        rightPadding: 30
    }

    Button {
        id: searchButton
        objectName: "searchButton"
        text: "Search"
        enabled: !searchBarModel.isSearching && searchText.text.trim().length > 0
        onClicked: searchBarModel.performSearch()

        Layout.preferredWidth: 100
    }
}
