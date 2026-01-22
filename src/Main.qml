// src/Main.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import wikipedia_qt // For NavigationState, GlobalState, etc.
import wikipedia_qt.History
import wikipedia_qt.SearchBar
import wikipedia_qt.Sidebar

ApplicationWindow {
    id: root
    width: 800
    height: 600
    visible: true

    // Store created items for cleanup
    property var createdItems: []

    // Main layout
    ColumnLayout {
        anchors.fill: parent
        spacing: 1

        Header {
            stackView: stackView // Pass stackView to Header
        }

        // StackView for navigation - removed Layout.fillHeight/Width
        StackView {
            id: stackView
            // Let the StackView manage its own size within the ColumnLayout
            Layout.fillWidth: true
            Layout.fillHeight: true
            initialItem: searchView
        }
    }

    // Define the search view (root view)
    Component {
        id: searchView
        SearchScreen {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }

    // Define the article view component
    Component {
        id: articleComponent
        MainContent {
            onBackRequested: function () {
                if (stackView.depth > 1) {
                    stackView.pop();
                }
            }
        }
    }

    // Define the history view component
    Component {
        id: historyComponent
        History {}
    }

    // Set NavigationState.stackView when StackView is ready
    Component.onCompleted: {
        NavigationState.setStackView(stackView);
        // Install the event filter on the root ApplicationWindow
        NavigationState.installEventFilter(root);

        // Connect navigation signals
        NavigationState.navigateToContentRequested.connect(() => {
            if (!(stackView.currentItem && stackView.currentItem instanceof MainContent)) {
                var articleItem = articleComponent.createObject(stackView);
                root.createdItems.push(articleItem);
                stackView.pop(); // Clear the stack before pushing
                stackView.push(articleItem);
            }
        });

        NavigationState.navigateToHistoryRequested.connect(() => {
            if (!(stackView.currentItem && stackView.currentItem instanceof History)) {
                const historyItem = historyComponent.createObject(stackView);
                root.createdItems.push(historyItem);
                stackView.pop(); // Clear the stack before pushing
                stackView.push(historyItem);
            }
        });

        NavigationState.navigateToSearchRequested.connect(() => {
            while (stackView.depth > 1) {
                stackView.pop();
            }
        });

        // Connect the backButtonPressed signal to handle back navigation
        NavigationState.backButtonPressed.connect(() => {
            if (stackView.depth > 1) {
                stackView.pop();
            }
        });
    }

    // Listen for changes to NavigationState.currentView
    QtObject {
        id: connections
        property bool currentViewConnected: false

        Component.onCompleted: {
            currentViewConnected = true;
        }

        Component.onDestruction: {
            if (currentViewConnected) {
                // No-op
            }
        }

        function onCurrentViewChanged() {
            switch (NavigationState.currentView) {
            case "content":
                // Only push if we're not already on the article view
                if (!(stackView.currentItem && stackView.currentItem instanceof MainContent)) {
                    // Create the component instance and push it
                    var articleItem = articleComponent.createObject(stackView);
                    // Store reference for cleanup
                    root.createdItems.push(articleItem);
                    stackView.push(articleItem);
                }
                break;
            case "history":
                // Only push if we're not already on the history view
                if (!(stackView.currentItem && stackView.currentItem instanceof History)) {
                    // Create the component instance and push it
                    var historyItem = historyComponent.createObject(stackView);
                    // Store reference for cleanup
                    root.createdItems.push(historyItem);
                    stackView.push(historyItem);
                }
                break;
            case "search":
                // Return to search view (root)
                while (stackView.depth > 1) {
                    stackView.pop();
                }
                break;
            default:
                // For any other case, ensure we're at the root
                while (stackView.depth > 1) {
                    stackView.pop();
                }
                break;
            }
        }
    }
}
