// Header.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt
import wikipedia_qt.Header

ToolBar {
    id: header
    Layout.fillWidth: true

    topPadding: 10
    bottomPadding: 10

    property StackView stackView: null
    property SystemPalette sysPalette: SystemPalette {}

    signal changeView(string viewName)

    HeaderModel {
        id: headerModel
    }

    RowLayout {
        ToolButton {
            id: backButton
            text: "Back"
            Layout.fillHeight: true

            contentItem: Image {
                source: "image://svg/back"
                fillMode: Image.PreserveAspectFit
                sourceSize: Qt.size(24, 24)
            }
            onClicked: {
                if (header.stackView && header.stackView.depth > 1) {
                    header.stackView.pop();
                }
            }
            visible: header.stackView ? header.stackView.depth > 1 : false
        }
        ToolButton {
            height: parent.height
            contentItem: Image {
                source: "image://svg/home"
                fillMode: Image.PreserveAspectFit
                sourceSize: Qt.size(24, 24)
            }
            text: "Home"
            onClicked: {
                header.changeView(Constants.searchView);
            }
        }
        ToolButton {
            contentItem: Image {
                source: "image://svg/history"
                fillMode: Image.PreserveAspectFit
                sourceSize: Qt.size(24, 24)
            }
            text: "History"
            onClicked: header.changeView(Constants.historyView)
        }
    }
}

