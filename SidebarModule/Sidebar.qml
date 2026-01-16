// Sidebar.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Sidebar 1.0

ColumnLayout {
    id: sidebar
    Layout.fillHeight: true
    Layout.preferredWidth: 200
    Layout.minimumWidth: 40
    Layout.maximumWidth: 200
    spacing: 10
    
    SidebarModel {
        id: sidebarModel
    }

    Button {
        text: "Home"
        Layout.fillWidth: true
        onClicked: sidebarModel.navigateTo("home")
    }

    Button {
        text: "History"
        Layout.fillWidth: true
        onClicked: sidebarModel.navigateTo("history")
    }

    Button {
        text: "Bookmarks"
        Layout.fillWidth: true
        onClicked: sidebarModel.navigateTo("bookmarks")
    }
}
