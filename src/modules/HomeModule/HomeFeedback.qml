import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root

    property int status: 0
    property string errorMessage: ""
    property string emptyMessage: "No items available."
    property string sectionName: "section"

    signal retryRequested()

    visible: status === 1 || status === 3 || status === 4

    BusyIndicator {
        objectName: root.sectionName + "LoadingIndicator"
        running: root.status === 1
        visible: running
        Layout.alignment: Qt.AlignHCenter
    }

    Label {
        objectName: root.sectionName + "Status"
        visible: root.status === 3 || root.status === 4
        text: root.status === 3 ? root.emptyMessage : root.errorMessage
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }

    Button {
        objectName: root.sectionName + "RetryButton"
        visible: root.status === 3 || root.status === 4
        text: "Retry"
        onClicked: root.retryRequested()
    }
}
