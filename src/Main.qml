// src/Main.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import wikipedia_qt // For NavigationState, GlobalState, etc.
import wikipedia_qt.History
import wikipedia_qt.ContentDisplay
import wikipedia_qt.Header

ApplicationWindow {
    id: root
    width: 800
    height: 600
    visible: true

    // Main layout
    ColumnLayout {
        anchors.fill: parent
        spacing: 1

        Header {
            stackView: stackView // Pass stackView to Header
            onChangeView: function (viewName) {
                NavigationState.setCurrentView(viewName);
            }
        }

        // StackView for navigation
        StackView {
            id: stackView
            Layout.fillWidth: true
            Layout.fillHeight: true
            initialItem: searchView

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
                ContentDisplay {
                    articleText: GlobalState.currentPageExtract
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
        }
    }

    // Set NavigationState.stackView when StackView is ready
    Component.onCompleted: function () {
        NavigationState.setStackView(stackView);
        NavigationState.addView(Constants.searchView, searchView);
        NavigationState.addView(Constants.contentView, articleComponent);
        NavigationState.addView(Constants.historyView, historyComponent);
        // Install the event filter on the root ApplicationWindow
        NavigationState.installEventFilter(root);

        // Connect the backButtonPressed signal to handle back navigation
        NavigationState.backButtonPressed.connect(() => {
            if (stackView.depth > 1) {
                stackView.pop();
            }
        });

        NavigationState.pushView.connect(view => {
            stackView.push(view);
        });

        NavigationState.replaceView.connect((newView) => {
            // Clear the stack and replace with the new view instantly
            stackView.replace(null, newView);
        })
    }
}
