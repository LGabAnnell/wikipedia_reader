// MainContent.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt 1.0

Frame {
    Layout.fillWidth: true
    Layout.fillHeight: true
    padding: 0

    // Loading indicator
    BusyIndicator {
        id: loadingIndicator
        anchors.centerIn: parent
        running: GlobalState.isLoading
        visible: GlobalState.isLoading
    }

    Frame {
        Text {
            text: "Main Content"
        }
    }
}
