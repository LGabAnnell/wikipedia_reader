// ContentDisplay.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt
import wikipedia_qt.ContentDisplay 1.0
import wikipedia_qt.Section 1.0

Item {
    id: mainContent
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    signal backRequested
    property string articleText: ""
    property bool sectionsPanelVisible: false

    onArticleTextChanged: {
        scrollView.ScrollBar.vertical.position = 0;
    }
    
    // Fetch sections when page changes
    Connections {
        target: GlobalState
        function onCurrentPageChanged() {
            // Reset sections panel visibility when page changes
            mainContent.sectionsPanelVisible = false
        }
    }

    BusyIndicator {
        id: loadingIndicator
        anchors.centerIn: parent
        running: GlobalState.isLoading
        visible: GlobalState.isLoading
    }

    Shortcut {
        sequences: [StandardKey.Find]
        onActivated: {
            searchBar.visible = true;
            searchField.forceActiveFocus();
        }
    }

    Shortcut {
        sequences: ["Escape"]
        onActivated: {
            searchBar.visible = false;
        }
    }

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        spacing: 0

        RowLayout {
            id: searchBar
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            spacing: 10
            z: 1
            visible: false

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: "Search..."
                z: 1
                onTextChanged: function () {
                    contentDisplay.performSearch(text, articleSection.getText(0, articleSection.text.length));
                }

                Keys.onReturnPressed: function (event) {
                    if (event.modifiers & Qt.ShiftModifier) {
                        contentDisplay.navigateToPreviousResult();
                        return;
                    }
                    
                    contentDisplay.navigateToNextResult();
                }
            }

            Text {
                text: contentDisplay.totalResults > 0 ? contentDisplay.currentResultIndex + " of " + contentDisplay.totalResults : ""
                color: "white"
                font.pixelSize: 14
                verticalAlignment: Text.AlignVCenter
            }

            Button {
                id: upButton
                text: "↑"
                onClicked: {
                    contentDisplay.navigateToPreviousResult();
                }
            }

            Button {
                id: downButton
                text: "↓"
                onClicked: {
                    contentDisplay.navigateToNextResult();
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            spacing: 10

            Button {
                id: galleryButton
                text: "Gallery"
                Layout.alignment: Qt.AlignRight
                visible: GlobalState.currentPageTitle.length > 0
                onClicked: {
                    NavigationState.navigateToImageGallery();
                }
            }

            Button {
                id: copyHtmlButton
                text: "Copy HTML"
                Layout.alignment: Qt.AlignRight
                visible: GlobalState.currentPageTitle.length > 0 && mainContent.articleText.length > 0
                onClicked: {
                    GlobalState.copyToClipboard(mainContent.articleText)
                }
            }

            Button {
                id: sectionsButton
                text: "Sections"
                Layout.alignment: Qt.AlignRight
                visible: GlobalState.currentPageTitle.length > 0
                onClicked: {
                    mainContent.sectionsPanelVisible = !mainContent.sectionsPanelVisible
                }
            }
        }

        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ScrollBar.vertical.interactive: true
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn

            Column {
                id: articleDisplay
                spacing: 20
                padding: 20
                width: scrollView.width - scrollView.effectiveScrollBarWidth - leftPadding - rightPadding

                property SystemPalette sysPalette: SystemPalette {}

                TextEdit {
                    text: GlobalState.currentPageTitle
                    font.pixelSize: 20
                    font.bold: true
                    color: articleDisplay.sysPalette.text
                    selectionColor: articleDisplay.sysPalette.highlight
                    visible: GlobalState.currentPageTitle.length > 0
                    width: parent.width
                    wrapMode: TextEdit.Wrap
                    readOnly: true
                    selectByMouse: true
                }

                TextEdit {
                    id: articleSection
                    text: mainContent.articleText
                    selectionColor: articleDisplay.sysPalette.highlight
                    wrapMode: TextEdit.Wrap
                    font.pixelSize: 14
                    textFormat: TextEdit.RichText
                    color: articleDisplay.sysPalette.text
                    visible: mainContent.articleText.length > 0
                    width: parent.width
                    readOnly: true
                    selectByMouse: true
                    ContextMenu.menu: Menu {
                        MenuItem {
                            text: "Copy"
                            onTriggered: articleSection.copy()
                        }
                    }

                    onLinkHovered: {}
                    onLinkActivated: function (link) {
                        if (link.startsWith("/wiki/")) {
                            var title = link.substring(6).replace(/_/g, " ");
                            GlobalState.loadArticleByTitle(title);
                        }
                    }
                }

                Text {
                    text: "Select an article to view its content"
                    wrapMode: Text.WordWrap
                    color: articleDisplay.sysPalette.text
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    visible: GlobalState.currentPageTitle.length === 0 && !GlobalState.isLoading
                    width: parent.width
                }

                Text {
                    text: GlobalState.errorMessage
                    color: "red"
                    visible: GlobalState.errorMessage.length > 0
                    width: parent.width
                    wrapMode: Text.WordWrap
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            function scrollToCursor() {
                // Get the cursor rectangle in the TextEdit's coordinate system
                const cursorRect = articleSection.cursorRectangle;
                // Calculate the position to scroll to
                let scrollToY = cursorRect.y - scrollView.height / 2;
                // Ensure the position is within valid bounds
                scrollToY = Math.max(0, Math.min(scrollToY, scrollView.contentHeight - scrollView.height));
                // Use the ScrollBar's value property to set the scroll position
                scrollView.ScrollBar.vertical.position = scrollToY / (scrollView.contentHeight - scrollView.height);
            }
        }
    }

    ContentDisplayModel {
        id: contentDisplay
        onSearchResultsAvailable: function (indices) {
            if (indices.length === 0) {
                articleSection.select(0, 0);
                return;
            }

            articleSection.cursorPosition = indices[0].start;
            articleSection.select(indices[0].start, indices[0].end);

            // Manually set the contentY property of the Flickable
            scrollView.scrollToCursor();
        }

        onNavigateToResult: function (start, end) {
            articleSection.cursorPosition = start;
            articleSection.select(start, end);
            scrollView.scrollToCursor();
        }
    }

    // Sections overlay panel
    Rectangle {
        id: sectionsOverlay
        width: mainContent.width * 0.5
        height: mainContent.height
        color: "#1e1e1e"
        anchors.right: mainContent.right
        anchors.top: mainContent.top
        anchors.bottom: mainContent.bottom
        z: 100
        visible: mainContent.sectionsPanelVisible
        border.color: "#333333"
        border.width: 1
        
        x: mainContent.sectionsPanelVisible ? 0 : width
        
        // Animation for sliding in/out
        PropertyAnimation {
            id: slideAnimation
            target: sectionsOverlay
            property: "x"
            to: mainContent.sectionsPanelVisible ? 0 : width
            duration: 300
            easing.type: Easing.InOutQuad
        }
        
        // Close button
        Button {
            id: closeSectionsButton
            text: "×"
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 10
            width: 40
            height: 40
            onClicked: mainContent.sectionsPanelVisible = false
            z: 101
            background: Rectangle {
                color: hovered ? "#333333" : "#2e2e2e"
                radius: 5
            }
            contentItem: Text {
                text: "×"
                color: "white"
                font.pixelSize: 18
            }
        }
        
        // Section component
        Section {
            id: sectionComponent
            anchors.fill: parent
            anchors.margins: 50
            anchors.topMargin: 50
            anchors.bottomMargin: 10
            
            onSectionClicked: function(section) {
                mainContent.sectionsPanelVisible = false
                // Scroll to section anchor if it exists in the article
                if (section.anchor && articleSection.text.includes(section.anchor)) {
                    var anchorIndex = articleSection.text.indexOf("<a name=\"" + section.anchor + "\">")
                    if (anchorIndex !== -1) {
                        articleSection.cursorPosition = anchorIndex
                        scrollView.ScrollBar.vertical.position = 0
                        // Scroll to the section
                        var scrollPos = anchorIndex / articleSection.text.length
                        scrollView.ScrollBar.vertical.position = scrollPos
                    }
                }
            }
        }
    }

    // Close sections panel on escape key when visible
    Shortcut {
        sequences: ["Escape"]
        onActivated: {
            if (mainContent.sectionsPanelVisible) {
                mainContent.sectionsPanelVisible = false
            } else if (searchBar.visible) {
                searchBar.visible = false
            }
        }
    }
}
