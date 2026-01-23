// History.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt 1.0

Item {
    id: container
    // Removed anchors.fill: parent to avoid conflicting with StackView
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    clip: true
    property list<history_item> history: HistoryState.history

    // Error message dialog
    Dialog {
        id: errorDialog
        standardButtons: Dialog.Ok
        title: "Database Error"
        modal: true
        visible: false
        Label {
            id: errorMessage
            text: ""
        }
    }

    Component.onCompleted: {
        // Connect to database error signal
        HistoryState.databaseError.connect(function (errorMessage) {
            errorMessage.text = errorMessage;
            errorDialog.visible = true;
            console.warn("Database error:", errorMessage);
        });
    }

    ColumnLayout {
        spacing: 10
        // Use the container's dimensions instead of anchoring
        width: container.width
        height: container.height
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
        // Empty state message
        Label {
            id: emptyStateLabel
            text: "No browsing history yet"
            color: "gray"
            font.pixelSize: 14
            horizontalAlignment: Text.AlignHCenter
            visible: container.history.length === 0
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 20
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
            visible: container.history.length > 0

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
                        anchors.fill: parent
                        onEntered: function () {
                            bgRect.color.a = .5;
                            bgRect.border.color = sysPalette.highlight;
                            bgRect.border.width = 2;
                        }
                        onExited: function () {
                            bgRect.border.width = 0;
                        }
                        onClicked: function () {
                            historyList.currentIndex = index;
                            // Load the article when clicked
                            if (modelData.pageId > 0) {
                                GlobalState.loadArticleByPageId(modelData.pageId);
                                // Get the StackView from NavigationState and push the article view
                                NavigationState.navigateToContent();
                            }
                        }
                    }
                }
            }
        }
    }
}
