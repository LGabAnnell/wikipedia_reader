// Header.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt

ToolBar {
    id: header
    Layout.fillWidth: true

    // Accept stackView as a property
    property StackView stackView: null

    signal changeView(string viewName)

    height: 50

    RowLayout {
        ToolButton {
            contentItem: Image {
                source: "qrc:/icons/back.jpg"
                fillMode: Image.PreserveAspectFit
                sourceSize: Qt.size(24, 24)
            }
            text: "Back"
            onClicked: {
                if (stackView && stackView.depth > 1) {
                    stackView.pop();
                }
            }
        }
        ToolButton {
            height: parent.height
            contentItem: Image {
                source: "qrc:/icons/home.jpg"
                fillMode: Image.PreserveAspectFit
                // Set the source size to match the desired siz
                sourceSize: Qt.size(24, 24)
            }
            text: "Home"
            onClicked: {
                header.changeView(Constants.searchView)
            }
        }
        ToolButton {
            contentItem: Image {
                source: "qrc:/icons/history.svg"
                fillMode: Image.PreserveAspectFit
                sourceSize: Qt.size(24, 24)
            }
            text: "History"
            onClicked: header.changeView(Constants.historyView)
        }
        ToolButton {
            contentItem: Image {
                source: "qrc:/icons/bookmark.jpg"
                fillMode: Image.PreserveAspectFit
                sourceSize: Qt.size(24, 24)
            }
            text: "Bookmarks"
        }
        ToolButton {
            contentItem: Image {
                source: "qrc:/icons/safes.jpg"
                fillMode: Image.PreserveAspectFit
                sourceSize: Qt.size(24, 24)
            }
            text: "Safes"
        }
        ToolButton {
            contentItem: Image {
                source: "qrc:/icons/location.png"
                fillMode: Image.PreserveAspectFit
                sourceSize: Qt.size(24, 24)
            }
            text: "Location"
        }
        // Item {
        //     Layout.fillWidth: true
        // }
    }

    // Internal alias to avoid naming conflicts
    property QtObject stackViewAlias: null
}

