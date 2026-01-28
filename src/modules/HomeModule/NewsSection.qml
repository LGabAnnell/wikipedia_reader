import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import wikipedia_qt

Column {
    id: newsSection
    spacing: 10
    padding: 20

    property alias model: repeater.model

    Label {
        text: "In the news"
        font.pixelSize: 20
        font.bold: true
    }

    // News items list
    ColumnLayout {
        id: newsItemsColumn
        spacing: 15

        Repeater {
            id: repeater
            delegate: Item {
                Layout.preferredHeight: rowLayout.implicitHeight
                Layout.preferredWidth: newsSection.width - 20

                RowLayout {
                    property SystemPalette sysPalette: SystemPalette {}
                    id: rowLayout
                    spacing: 15
                    width: parent.width

                    // News image if available
                    Image {
                        visible: modelData.imageUrl && modelData.imageUrl !== ""
                        width: 80
                        height: 60
                        fillMode: Image.PreserveAspectFit
                        source: modelData.imageUrl || "qrc:/images/news_placeholder.jpg"
                        clip: true
                        sourceSize: Qt.size(80, 60)
                    }

                    ColumnLayout {
                        id: columnNews
                        spacing: 5

                        Label {
                            text: "• " + modelData.title
                            font.pixelSize: 14
                            font.bold: true
                            color: "#002bb8"
                            wrapMode: Text.WordWrap
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    GlobalState.currentPageTitle = modelData.title;
                                    GlobalState.currentPageExtract = modelData.description;
                                    GlobalState.currentPageUrl = modelData.url;
                                    NavigationState.navigateToView(Constants.contentView());
                                }
                            }
                        }

                        TextEdit {
                            id: newsDescription
                            text: modelData.description
                            readOnly: true
                            selectByKeyboard: true
                            selectByMouse: true
                            font.pixelSize: 14
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                            color: rowLayout.sysPalette.text
                        }
                    }
                }
            }
        }
    }
}