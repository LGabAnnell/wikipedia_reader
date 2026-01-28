// src/modules/HomeModule/OnThisDaySection.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt

Column {
    id: onThisDaySection
    Layout.fillWidth: true
    spacing: 10
    padding: 20

    property alias model: repeater.model

    Label {
        Layout.fillWidth: true
        text: "On this day"
        font.pixelSize: 20
        font.bold: true
    }

    // On this day events
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 10
        Repeater {
            id: repeater
            delegate: RowLayout {
                Layout.fillWidth: true
                spacing: 15

                Label {
                    text: modelData.year
                    font.pixelSize: 14
                    font.bold: true
                    Layout.alignment: Qt.AlignTop
                    width: 60
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5

                    Label {
                        Layout.fillWidth: true
                        text: "• " + modelData.event
                        font.pixelSize: 14
                        wrapMode: Text.Wrap

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                GlobalState.currentPageTitle = "On this day: " + modelData.year;
                                GlobalState.currentPageExtract = modelData.event;
                                GlobalState.currentPageUrl = modelData.url;
                                NavigationState.navigateToView(Constants.contentView());
                            }
                        }
                    }
                }
            }
        }
    }
}
