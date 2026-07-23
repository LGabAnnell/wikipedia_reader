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
    property SystemPalette sysPalette: SystemPalette {}

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

                    TextEdit {
                        Layout.fillWidth: true
                        text: "• <a href=\"page://" + modelData.pageid + "\">" + modelData.event + "</a>"
                        font.pixelSize: 14
                        wrapMode: TextEdit.Wrap
                        textFormat: TextEdit.RichText
                        color: onThisDaySection.sysPalette.text
                        readOnly: true
                        selectByMouse: true
                        onLinkActivated: function(link) {
                            if (link.startsWith("page://")) {
                                var pageId = parseInt(link.substring(7));
                                if (pageId > 0) {
                                    GlobalState.loadArticleByPageId(pageId);
                                    NavigationState.navigateToContent();
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
