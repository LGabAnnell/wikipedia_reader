// Header.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ToolBar {
    id: header
    Layout.fillWidth: true

    height: 50

    RowLayout {
        ToolButton {
            height: parent.height
            contentItem: Image {
                source: "qrc:/icons/home.jpg"
                fillMode: Image.PreserveAspectFit
                // Set the source size to match the desired siz
                sourceSize: Qt.size(24, 24)
            }
            text: "Home"
            onClicked: GlobalState.setCurrentView("content")
        }
        ToolButton {
            contentItem: Image {
                source: "qrc:/icons/history.jpg"
                fillMode: Image.PreserveAspectFit
                sourceSize: Qt.size(24, 24)
            }
            text: "History"
            onClicked: GlobalState.setCurrentView("history")
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
}
