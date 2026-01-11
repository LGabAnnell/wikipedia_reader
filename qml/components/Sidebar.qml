// Sidebar.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    Layout.preferredWidth: 200
    Layout.fillHeight: true
    background: Rectangle {
        color: "#333333"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Label {
            text: "Results"
            font.pixelSize: 16
            padding: 10
            horizontalAlignment: Text.AlignHCenter
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
                onClicked: console.log("Selected:", model.title)
            }
        }
    }
}