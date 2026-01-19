// History.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt 1.0

Item {
    id: container
    clip: true
    anchors.fill: parent

    property list<history_item> history: HistoryState.history

    ColumnLayout {
        spacing: 10
        anchors.fill: parent

        // Header with title and clear button
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 10
            Layout.rightMargin: 10
            Layout.leftMargin: 10
            spacing: 10

            Label {
                text: "Browsing History"
                font.pixelSize: 18
                font.bold: true
                Layout.alignment: Qt.AlignLeft
            }

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: "Clear History"
                onClicked: HistoryState.clearHistory()
                enabled: container.history.length > 0
            }
        }

        // History list
        GridView {
            id: historyList
            Layout.fillHeight: true
            Layout.fillWidth: true
            model: container.history
            clip: true
            cellHeight: 60
            cellWidth: 140

            delegate: Item {
                width: 140
                height: 60
                ItemDelegate {
                    id: delegate
                    SystemPalette {
                        id: sysPalette
                    }
                    hoverEnabled: true
                    padding: 10
                    width: 120
                    height: 60
                    anchors.centerIn: parent
                    clip: true

                    background: Rectangle {
                        id: bgRect
                        color: sysPalette.button
                        radius: 5
                    }

                    Rectangle {
                        id: selectedRect
                        property SystemPalette sysPalette: sysPalette
                        visible: historyList.currentIndex == index
                        color: sysPalette.highlight
                        border.color: sysPalette.highlight
                        border.width: 2
                        radius: 5
                    }

                    Column {
                        Layout.alignment: Qt.AlignHCenter
                        padding: 10
                        spacing: 5
                        clip: true
                        Text {
                            text: modelData.title
                            color: sysPalette.text
                            font.pixelSize: 14
                            elide: Text.ElideRight
                        }
                        Text {
                            text: modelData.timestamp.toString("hh:mm:ss - MMM d, yyyy")
                            color: sysPalette.text
                            font.pixelSize: 12
                            opacity: 0.7
                            elide: Text.ElideRight
                        }
                    }

                    MouseArea {
                        hoverEnabled: true
                        onEntered: function () {
                            if (historyList.currentIndex == index) {
                                return;
                            }
                            parent.background.color = sysPalette.highlight;
                            bgRect.color.a = .5;
                            bgRect.border.color = sysPalette.highlight;
                            bgRect.border.width = 2;
                        }
                        onExited: function () {
                            parent.background.color = sysPalette.button;
                            bgRect.border.width = 0;
                        }
                        onPressed: function () {
                            historyList.currentIndex = index;
                            // Load the article when clicked
                            if (modelData.pageId > 0) {
                                GlobalState.loadArticleByPageId(modelData.pageId);
                                // Switch back to content view
                                GlobalState.setCurrentView("content");
                            }
                        }
                    }
                }
            }

            // Empty state message
            // footer: Text {
            //     text: HistoryState.history.length() === 0 ? "No browsing history yet" : ""
            //     color: "gray"
            //     font.pixelSize: 14
            //     horizontalAlignment: Text.AlignHCenter
            //     visible: HistoryState.history.length() === 0
            //     anchors.horizontalCenter: parent.horizontalCenter
            //     padding: 20
            // }
        }
    }
}
