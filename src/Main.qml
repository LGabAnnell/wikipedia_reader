// Main.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt 1.0
import wikipedia_qt.SearchBar 1.0
import wikipedia_qt.Sidebar 1.0
import wikipedia_qt.History 1.0

ApplicationWindow {
    id: root
    visible: true
    width: 800
    height: 600
    title: "Browse Wikipedia"

    header: Header {
        Layout.fillWidth: true
    }

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: searchComponent

        // Listen for changes to GlobalState.currentView
        Connections {
            target: GlobalState
            function onCurrentViewChanged() {
                switch (GlobalState.currentView) {
                case "content":
                    stackView.replace(articleComponent)
                    break;
                case "history":
                    stackView.replace(historyComponent)
                    break;
                default:
                    stackView.replace(searchComponent)
                    break;
                }
            }
        }
    }

    // Components for StackView
    Component {
        id: searchComponent
        SearchScreen {}
    }

    Component {
        id: articleComponent
        MainContent {}
    }

    Component {
        id: historyComponent
        History {}
    }
}

