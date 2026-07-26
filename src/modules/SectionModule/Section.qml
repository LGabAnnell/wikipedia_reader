// Section.qml - Displays the table of contents sections for a Wikipedia article
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt
import wikipedia_qt.Section 1.0

Item {
    id: root
    width: parent ? parent.width : 300
    height: parent ? parent.height : 600
    
    property bool loading: sectionModel.isLoading
    property var sections: sectionModel.sections
    signal sectionClicked(var section)
    
    SectionModel {
        id: sectionModel
    }
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 5
        
        // Header
        Label {
            text: "Sections"
            font.bold: true
            font.pixelSize: 18
            Layout.alignment: Qt.AlignHCenter
            Layout.margins: 10
            color: "white"
        }
        
        // Loading indicator
        BusyIndicator {
            id: loadingIndicator
            running: sectionModel.isLoading
            visible: sectionModel.isLoading
            Layout.alignment: Qt.AlignHCenter
        }
        
        // Error message
        Text {
            text: sectionModel.errorMessage
            color: "red"
            visible: sectionModel.errorMessage.length > 0
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.margins: 10
        }
        
        // Container for sections list and overlay
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            ListView {
                id: sectionsList
                anchors.fill: parent
                model: sectionModel.sections
                delegate: ItemDelegate {
                    text: modelData.title
                    font.pixelSize: 14 - (modelData.level > 2 ? modelData.level - 2 : 0)
                    padding: 10
                    leftPadding: 10 + (modelData.level - 1) * 20
                    onClicked: root.sectionClicked(modelData)
                    
                    contentItem: Text {
                        text: modelData.title
                        color: "white"
                        font.pixelSize: 14 - (modelData.level > 2 ? modelData.level - 2 : 0)
                        font.bold: modelData.level <= 2
                        elide: Text.ElideRight
                    }
                }
            }
            
            // Empty message overlay
            Rectangle {
                id: emptyMessage
                visible: sectionModel.sections.length === 0 && !sectionModel.isLoading
                anchors.fill: parent
                color: "transparent"
                z: 1
                
                Text {
                    text: "No sections found"
                    color: "#888888"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    anchors.fill: parent
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

