// ImageView.qml - Fullscreen image view pushed onto the navigation stack
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import wikipedia_qt // For GlobalState, NavigationState

Item {
    id: root
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0

    Rectangle {
        anchors.fill: parent
        color: "black"

        Image {
            id: fullImage
            anchors.fill: parent
            source: GlobalState.currentImageUrl
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            cache: true

            // Loading indicator
            Rectangle {
                anchors.centerIn: parent
                width: 40
                height: 40
                color: "transparent"
                visible: fullImage.status === Image.Loading

                BusyIndicator {
                    anchors.centerIn: parent
                    running: true
                    visible: true
                }
            }

            // Error placeholder
            Rectangle {
                anchors.centerIn: parent
                width: parent.width
                height: parent.height
                color: Qt.rgba(0, 0, 0, 0.1)
                visible: fullImage.status === Image.Error

                Text {
                    anchors.centerIn: parent
                    text: "Image not available"
                    color: "gray"
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        // Image description overlay at the bottom
        Rectangle {
            id: descriptionOverlay
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            color: Qt.rgba(0, 0, 0, 0.7)
            visible: GlobalState.currentImageDescription.length > 0
            height: descriptionColumn.implicitHeight + 2 * descriptionColumn.anchors.margins

            ColumnLayout {
                id: descriptionColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 12
                spacing: 4

                Text {
                    Layout.fillWidth: true
                    text: GlobalState.currentImageDescription
                    color: "white"
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                    elide: Text.ElideRight
                    maximumLineCount: 6
                    horizontalAlignment: Text.AlignLeft
                }
            }
        }
    }

    // Back button (top-left); pops the stack via NavigationState
    Button {
        id: backButton
        text: "Back"
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 10
        onClicked: {
            var sv = NavigationState.stackView;
            if (sv && sv.depth > 1) {
                sv.pop();
            }
        }
    }
}
