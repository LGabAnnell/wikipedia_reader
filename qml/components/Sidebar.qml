// Sidebar.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    Layout.preferredWidth: 200
    Layout.fillHeight: true
    padding: 0
    background: Rectangle {
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Label {
            text: "Results"
            font.pixelSize: 16
            font.bold: true
            padding: 10
            horizontalAlignment: Text.AlignHCenter
            background: Rectangle {
            }
        }

        ListView {
            id: sidebarList
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: ListModel {
                id: sidebarModel
                ListElement {
                    title: "Winiara Gdane"
                }
                ListElement {
                    title: "Sany's Sente"
                }
                ListElement {
                    title: "Acis Glairans"
                }
                ListElement {
                    title: "Eing Piores"
                }
                ListElement {
                    title: "Tonange Yeum Tapes"
                }
            }
            delegate: ItemDelegate {
                text: model.title
                width: parent.width
                padding: 10
                background: Rectangle {
                }
                onClicked: console.log("Selected:", model.title)
            }
        }
    }
}