// ImageGallery.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import wikipedia_qt.ImageDisplay

Item {
    id: root
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0

    ColumnLayout {
        anchors.fill: parent
        // spacing: 10
        Layout.leftMargin: 10
        Layout.rightMargin: 10
        Layout.topMargin: 10
        Layout.bottomMargin: 10

        // Header row with title and back button
        RowLayout {
            Layout.fillWidth: true
            // spacing: 10

            Button {
                id: backButton
                text: "Back"
                Layout.alignment: Qt.AlignLeft
                onClicked: {
                    if (stackView.depth > 1) {
                        stackView.pop();
                    }
                }
            }

            Text {
                id: titleText
                text: imageModel.articleTitle
                font.pixelSize: 20
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                elide: Text.ElideRight
            }

            Item {
                Layout.preferredWidth: 60 // Spacer to balance the back button
            }
        }

        // Scrollable grid view for images
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.vertical.interactive: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            Layout.topMargin: 10
            Layout.bottomMargin: 10

            GridView {
                id: imageGrid
                model: imageModel.imageUrls
                // cellWidth: parent.width / 3 - spacing * 2
                cellHeight: cellWidth
                // spacing: 10
                clip: true

                delegate: Rectangle {
                    id: imageCell
                    width: imageGrid.cellWidth
                    height: imageGrid.cellHeight
                    color: "transparent"
                    border.color: Qt.lighter("#808080", 1.5)
                    border.width: 1
                    radius: 4

                    Image {
                        id: imageItem
                        anchors.fill: parent
                        anchors.margins: 5
                        source: modelData
                        fillMode: Image.PreserveAspectFit
                        sourceSize: Qt.size(width, height)
                        asynchronous: true
                        cache: true

                        // Show loading indicator or placeholder
                        Rectangle {
                            anchors.centerIn: parent
                            width: 40
                            height: 40
                            color: "transparent"
                            visible: imageItem.status === Image.Loading

                            BusyIndicator {
                                anchors.centerIn: parent
                                running: true
                                visible: true
                            }
                        }

                        Rectangle {
                            anchors.centerIn: parent
                            width: parent.width
                            height: parent.height
                            color: Qt.rgba(0, 0, 0, 0.1)
                            visible: imageItem.status === Image.Error

                            Text {
                                anchors.centerIn: parent
                                text: "Image not available"
                                color: "gray"
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                    }

                    // Image title overlay on hover
                    Rectangle {
                        id: overlay
                        anchors.fill: parent
                        color: Qt.rgba(0, 0, 0, 0.7)
                        visible: mouseArea.containsMouse && imageModel.imageUrls.length > 0
                        radius: 4

                        Text {
                            anchors.centerIn: parent
                            text: modelData.split("/").pop()
                            color: "white"
                            font.pixelSize: 12
                            elide: Text.ElideMiddle
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                    }
                }
            }
        }
    }

    ImageHomeModel {
        id: imageModel
    }

    Component.onCompleted: {
        imageModel.loadImagesForCurrentPage();
    }
}
