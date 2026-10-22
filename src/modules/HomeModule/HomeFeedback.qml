import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import wikipedia_qt.Home

ColumnLayout {
    id: root

    property int status: HomeModel.Idle
    property string errorMessage: ""
    property string emptyMessage: qsTr("No items available.")
    property string sectionName: "section"

    signal retryRequested()

    visible: status === HomeModel.Loading || status === HomeModel.Empty || status === HomeModel.Error

    BusyIndicator {
        objectName: root.sectionName + "LoadingIndicator"
        running: root.status === HomeModel.Loading
        visible: running
        Layout.alignment: Qt.AlignHCenter
    }

    Label {
        objectName: root.sectionName + "Status"
        visible: root.status === HomeModel.Empty || root.status === HomeModel.Error
        text: root.status === HomeModel.Empty ? root.emptyMessage : root.errorMessage
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }

    Button {
        objectName: root.sectionName + "RetryButton"
        visible: root.status === HomeModel.Empty || root.status === HomeModel.Error
        text: qsTr("Retry")
        onClicked: root.retryRequested()
    }
}
