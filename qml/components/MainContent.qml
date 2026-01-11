// MainContent.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    Layout.fillWidth: true
    Layout.fillHeight: true
    padding: 0

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Label {
            text: "Quantum Mechanics"
            font.pixelSize: 24
            padding: 10
        }

        Label {
            text: "The veasbar shliciance so veond you se ofe'donod the progreessing o ddillercent fordsely cam..."
            wrapMode: Text.Wrap
            padding: 10
        }
    }
}