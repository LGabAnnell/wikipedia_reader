// src/modules/HomeModule/DidYouKnowSection.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt

Column {
    id: didYouKnowSection
    // Layout.fillWidth: true
    // width: parent.width
    spacing: 10
    padding: 20
    property SystemPalette sysPalette: SystemPalette {}

    property alias model: repeater.model

    Label {
        Layout.fillWidth: true  // Ensure the Label fills the width
        text: "Did you know ..."
        font.pixelSize: 20
        font.bold: true
    }

    // Did you know items
    ColumnLayout {
        Layout.fillWidth: true  // Ensure the ColumnLayout fills the width
        spacing: 10

        Repeater {
            id: repeater
            delegate: TextEdit {
                readOnly: true
                selectByMouse: true
                text: "• <a href=\"page://" + modelData.pageid + "\">" + modelData.text + "</a>"
                font.pixelSize: 14
                wrapMode: TextEdit.Wrap
                textFormat: TextEdit.RichText
                color: didYouKnowSection.sysPalette.text
                Layout.preferredWidth: didYouKnowSection.width
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
