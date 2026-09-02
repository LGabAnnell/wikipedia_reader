// Section.qml - Collapsible vertical bar displaying the table of contents for a Wikipedia article
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt
import wikipedia_qt.Section 1.0

Item {
    id: root

    property bool collapsed: true
    property int expandedWidth: 260
    property int collapsedWidth: 36

    property bool loading: sectionModel.isLoading
    property var sections: sectionModel.sections
    signal sectionClicked(var section)

    implicitHeight: parent ? parent.height : 600

    // Animate the layout's preferred width rather than the item's width.
    // The parent RowLayout positions and sizes this item based on
    // Layout.preferredWidth, so animating that property keeps the right edge
    // fixed and lets the bar expand/collapse leftward. The layout sets
    // `width` to match each frame, so children anchored to parent.right
    // stay in sync. Using implicitWidth (instead of width) lets the layout
    // own the geometry; a direct `width` binding would jump instantly and
    // desync from the animated Layout.preferredWidth.
    Behavior on Layout.preferredWidth {
        NumberAnimation { duration: 200; easing.type: Easing.InOutQuad }
    }

    SectionModel {
        id: sectionModel
    }

    SystemPalette {
        id: sysPalette
    }

    Rectangle {
        id: background

        anchors.fill: parent
        border.color: sysPalette.midlight
        border.width: 1
        color: sysPalette.window
    }

    // Toggle handle, always visible at the top-right of the bar
    Button {
        id: toggleButton

        Accessible.name: collapsed ? "Expand sections panel" : "Collapse sections panel"
        anchors.margins: 4
        anchors.right: parent.right
        anchors.top: parent.top
        height: 28
        text: collapsed ? "\u2039" : "\u203A"
        width: 28
        z: 2

        onClicked: root.collapsed = !root.collapsed
    }

    // Vertical label shown while collapsed
    Text {
        anchors.centerIn: parent
        color: sysPalette.text
        font.pixelSize: 12
        rotation: 90
        text: "Sections"
        visible: root.collapsed
    }

    // Expanded content
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        anchors.topMargin: 40
        visible: !root.collapsed

        Label {
            Layout.alignment: Qt.AlignHCenter
            Layout.margins: 6
            color: sysPalette.text
            font.bold: true
            font.pixelSize: 16
            text: "Sections"
        }

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            running: sectionModel.isLoading
            visible: sectionModel.isLoading
        }

        Text {
            Layout.fillWidth: true
            Layout.margins: 10
            color: "red"
            text: sectionModel.errorMessage
            visible: sectionModel.errorMessage.length > 0
            wrapMode: Text.WordWrap
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true

            ListView {
                id: sectionsList

                anchors.fill: parent
                clip: true
                model: sectionModel.sections

                delegate: ItemDelegate {
                    width: ListView.view.width

                    contentItem: Text {
                        color: sysPalette.text
                        elide: Text.ElideRight
                        font.bold: modelData.level <= 1
                        font.pixelSize: 14 - (modelData.level > 1 ? modelData.level - 1 : 0)
                        leftPadding: modelData.level > 1 ? modelData.level * 5 : 0
                        text: modelData.title
                    }
                    onClicked: root.sectionClicked(modelData)
                }
            }

            // Empty message overlay
            Rectangle {
                anchors.fill: parent
                color: "transparent"
                visible: sectionModel.sections.length === 0 && !sectionModel.isLoading

                Text {
                    anchors.fill: parent
                    color: "#888888"
                    horizontalAlignment: Text.AlignHCenter
                    text: "No sections found"
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    // Automatically fetch sections when the component is ready
    Component.onCompleted: {
        if (GlobalState.currentPageTitle.length > 0) {
            sectionModel.fetchSections(GlobalState.currentPageTitle)
        }
    }

    // Refetch sections when the current page changes
    Connections {
        target: GlobalState
        function onCurrentPageChanged() {
            if (GlobalState.currentPageTitle.length > 0) {
                sectionModel.fetchSections(GlobalState.currentPageTitle)
            } else {
                sectionModel.clearSections()
            }
        }
    }
}
