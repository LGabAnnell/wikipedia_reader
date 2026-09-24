// src/Main.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import wikipedia_qt // For NavigationState, GlobalState, etc.
import wikipedia_qt.History
import wikipedia_qt.ContentDisplay
import wikipedia_qt.ImageDisplay
import wikipedia_qt.Header
import wikipedia_qt.Home
import wikipedia_qt.Section

ApplicationWindow {
    id: root
    objectName: "mainWindow"
    width: 800
    height: 600
    visible: true

    // Main layout
    ColumnLayout {
        anchors.fill: parent
        spacing: 1

        Header {
            objectName: "applicationHeader"
            stackView: stackView // Pass stackView to Header
            onChangeView: function (viewName) {
                if (viewName === Constants.homeView) {
                    if (stackView.depth > 1) {
                        stackView.pop(null);
                    }
                } else {
                    NavigationState.navigateToView(viewName);
                }
            }
        }

        // StackView for navigation
        StackView {
            id: stackView
            objectName: "applicationStack"
            Layout.fillWidth: true
            Layout.fillHeight: true
            initialItem: homeComponent

            // Define the search view
            Component {
                id: searchView
                SearchScreen {
                    objectName: "searchView"
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

            Component {
                id: homeComponent
                HomeScreen {
                    objectName: "homeView"
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }
            }

            // Define the history view component
            Component {
                id: historyComponent
                History {
                    objectName: "historyView"
                }
            }

            // Define the image gallery view component
            Component {
                id: imageGalleryComponent
                ImageHome {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }
            }

            // Define the fullscreen image view component
            Component {
                id: imageViewComponent
                ImageView {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }
            }
        }
    }

    // Set NavigationState.stackView when StackView is ready
    Component.onCompleted: function () {
        NavigationState.setStackView(stackView);
        NavigationState.addView(Constants.searchView, searchView);
        NavigationState.addView(Constants.contentView, articleComponent);
        NavigationState.addView(Constants.historyView, historyComponent);
        NavigationState.addView(Constants.homeView, homeComponent);
        NavigationState.addView(Constants.imageGalleryView, imageGalleryComponent);
        NavigationState.addView(Constants.imageView, imageViewComponent);
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
        });
    }
}
