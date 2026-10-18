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

    property bool databaseReady: HistoryState.databaseReady
    Component.onCompleted: {
        if (HistoryState.lastDatabaseError.length > 0) {
            errorMessage.text = HistoryState.lastDatabaseError;
            errorDialog.open();
        }
    }

    // Error message dialog
    Dialog {
        id: errorDialog
        objectName: "historyErrorDialog"
        width: Math.min(560, container.width - 40)
        standardButtons: Dialog.Ok
        title: qsTr("Database Error")
        modal: true
        visible: false
        Label {
            id: errorMessage
            objectName: "historyErrorText"
            width: errorDialog.availableWidth
            wrapMode: Text.Wrap
            text: ""
        }
    }

    Connections {
        target: HistoryState
        function onDatabaseError(message) {
            errorMessage.text = message;
            errorDialog.open();
        }
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
                text: qsTr("Browsing History")
                font.pixelSize: 18
                font.bold: true
                Layout.alignment: Qt.AlignLeft
            }
            Item {
                Layout.fillWidth: true
            }

            Button {
                objectName: "clearHistoryButton"
                text: qsTr("Clear History")
                onClicked: HistoryState.clearHistory()
                enabled: container.history.length > 0
            }
        }
        // Empty state message
        Label {
            id: emptyStateLabel
            objectName: "historyUnavailableLabel"
            text: container.databaseReady ? qsTr("No browsing history yet")
                                          : qsTr("Browsing history is unavailable")
            color: "gray"
            font.pixelSize: 14
            horizontalAlignment: Text.AlignHCenter
            visible: !container.databaseReady || container.history.length === 0
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 20
        }

        // History list
        GridView {
            id: historyList
            objectName: "historyList"
            Layout.fillHeight: true
            Layout.fillWidth: true
            model: container.history
            clip: true
            cellHeight: 100
            cellWidth: 240
            visible: container.history.length > 0

            delegate: Item {
                width: historyList.cellWidth
                height: historyList.cellHeight
                ItemDelegate {
                    id: delegate
                    objectName: "historyEntry-" + index
                    enabled: modelData.language.length > 0 && modelData.pageId > 0
                    hoverEnabled: true
                    padding: 10
                    width: parent.width - 10
                    height: parent.height - 10
                    anchors.centerIn: parent
                    clip: true
                    SystemPalette { id: sysPalette }

                    background: Rectangle {
                        color: sysPalette.button
                        radius: 5
                        border.width: delegate.hovered && delegate.enabled ? 2 : 0
                        border.color: sysPalette.highlight
                    }

                    contentItem: Column {
                        spacing: 5
                        Text {
                            width: parent.width
                            text: modelData.title
                            color: sysPalette.text
                            font.pixelSize: 14
                            elide: Text.ElideRight
                        }
                        Text {
                            objectName: "historyEdition-" + index
                            width: parent.width
                            text: modelData.language.length > 0
                                ? qsTr("%1 Wikipedia").arg(modelData.language.toUpperCase())
                                : qsTr("Unknown edition")
                            color: sysPalette.text
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                        Text {
                            width: parent.width
                            text: Qt.formatDateTime(modelData.timestamp, "hh:mm:ss - MMM d, yyyy")
                            color: sysPalette.text
                            font.pixelSize: 12
                            opacity: 0.7
                            elide: Text.ElideRight
                        }
                    }
                    onClicked: {
                        // Cache hits can reorder the list synchronously.
                        const language = modelData.language;
                        const pageId = modelData.pageId;
                        historyList.currentIndex = index;
                        if (ArticleState.loadArticleFromHistory(language, pageId))
                            NavigationState.navigateToContent();
                    }
                }
            }
        }
    }
}
