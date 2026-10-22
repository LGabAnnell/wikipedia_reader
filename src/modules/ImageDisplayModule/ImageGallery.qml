// ImageGallery.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts
import QtCore
import wikipedia_qt // For ArticleState, NavigationState, Constants
import wikipedia_qt.ImageDisplay

Item {
    id: root
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    property int pageId: ArticleState.currentPageId

    Settings {
        location: Qt.resolvedUrl(StandardPaths.writableLocation(StandardPaths.AppConfigLocation)
            + "/settings.ini")
        category: "ImageDisplay"
        property alias fullscreen: fullscreenSwitch.checked
    }

    property SystemPalette sysPalette: SystemPalette { }

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

            Button {
                id: backButton
                objectName: "imageGalleryBackButton"
                text: qsTr("Back")
                Layout.alignment: Qt.AlignLeft
                onClicked: {
                    const ownerStack = root.StackView.view
                            || (root.parent ? root.parent.StackView.view : null);
                    if (ownerStack && ownerStack.depth > 1) {
                        ownerStack.pop();
                    }
                }
            }

            Text {
                id: titleText
                objectName: "galleryTitle"
                text: imageModel.articleTitle
                font.pixelSize: 20
                font.bold: true
                color: root.sysPalette.text
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                elide: Text.ElideRight
            }

            Switch {
                id: fullscreenSwitch
                objectName: "imageFullscreenSwitch"
                text: qsTr("Fullscreen")
                checked: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Open images in fullscreen; turn off to view within the app")
            }
        }

        BusyIndicator {
            objectName: "galleryLoadingIndicator"
            Layout.alignment: Qt.AlignHCenter
            running: imageModel.isLoading
            visible: running
        }

        Label {
            objectName: "galleryErrorMessage"
            Layout.alignment: Qt.AlignHCenter
            text: imageModel.errorMessage
            visible: text.length > 0
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
                    objectName: "imageGalleryCell-" + index
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
                                text: qsTr("Image not available")
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
                        objectName: "imageGalleryMouseArea-" + index
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (index < 0 || index >= imageModel.imageUrls.length)
                                return;
                            imageModel.selectImage(index);
                            if (fullscreenSwitch.checked)
                                imageWindow.showFullScreen();
                            else
                                NavigationState.navigateToView(Constants.imageView);
                        }
                    }
                }
            }
        }
    }

    ImageHomeModel {
        id: imageModel
    }

    ImageFullscreenWindow {
        id: imageWindow
        transientParent: root.Window.window
    }

    onVisibleChanged: {
        if (!visible)
            imageWindow.close();
    }

    Component.onCompleted: {
        imageModel.loadImagesForPage(pageId);
    }
}
