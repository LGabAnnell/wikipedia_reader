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
                text: "• " + modelData.text
                font.pixelSize: 14
                wrapMode: Text.Wrap
                color: didYouKnowSection.sysPalette.text
                Layout.preferredWidth: didYouKnowSection.width
                // Layout.fillWidth: true  // Ensure the TextEdit fills the width
                // width: parent.width  // Explicitly set width to parent width
                // MouseArea {
                //     anchors.fill: parent
                //     cursorShape: Qt.PointingHandCursor
                //     onClicked: {
                //         GlobalState.currentPageTitle = "Did you know: " + modelData.text;
                //         GlobalState.currentPageExtract = modelData.text;
                //         GlobalState.currentPageUrl = modelData.url;
                //         NavigationState.navigateToView(Constants.contentView());
                //     }
                // }
            }
        }
    }
}
