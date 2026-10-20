// Image content shared by the navigation view and fullscreen window.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import wikipedia_qt // For ImageSelectionState

Item {
    id: root
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0

    property string backButtonText: qsTr("Back")
    signal backRequested()

    Shortcut {
        sequence: "Right"
        // WindowShortcut is dispatched only by the active window, even when a
        // child control such as the Back button owns focus.
        context: Qt.WindowShortcut
        enabled: root.visible
        onActivated: ImageSelectionState.selectNextImage()
    }

    Shortcut {
        sequence: "Left"
        context: Qt.WindowShortcut
        enabled: root.visible
        onActivated: ImageSelectionState.selectPreviousImage()
    }

    Rectangle {
        anchors.fill: parent
        color: "black"

        Image {
            id: fullImage
            objectName: "fullScreenImage"
            anchors.fill: parent
            source: width > 0 && height > 0 ? ImageSelectionState.currentImageUrl : ""
            fillMode: Image.PreserveAspectFit
            // Bound decoding and SVG rasterization to the viewer size instead
            // of allocating buffers at the source's intrinsic dimensions.
            sourceSize: Qt.size(Math.max(1, Math.ceil(width)),
                                Math.max(1, Math.ceil(height)))
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
            objectName: "imageCaptionOverlay"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            color: Qt.rgba(0, 0, 0, 0.7)
            visible: ImageSelectionState.currentImageDescription.length > 0
            height: descriptionColumn.implicitHeight + 2 * descriptionColumn.anchors.margins

            ColumnLayout {
                id: descriptionColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 12
                spacing: 4

                Text {
                    objectName: "imageCaptionText"
                    Layout.fillWidth: true
                    text: ImageSelectionState.currentImageDescription
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

    // The owner decides whether Back closes a window or pops a view.
    Button {
        id: backButton
        objectName: "imageBackButton"
        text: root.backButtonText
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 10
        onClicked: root.backRequested()
    }
}
