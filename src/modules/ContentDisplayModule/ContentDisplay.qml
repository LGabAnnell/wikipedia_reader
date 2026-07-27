// ContentDisplay.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt
import wikipedia_qt.ContentDisplay 1.0
import wikipedia_qt.Section 1.0

Item {
    id: mainContent

    property string articleText: ""
    property bool sectionsPanelVisible: false

    signal backRequested

    height: parent ? parent.height : 0
    width: parent ? parent.width : 0

    onArticleTextChanged: {
        scrollView.ScrollBar.vertical.position = 0;
    }

    // Fetch sections when page changes
    Connections {
        function onCurrentPageChanged() {
            // Reset sections panel visibility when page changes
            mainContent.sectionsPanelVisible = false;
        }

        target: GlobalState
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
            Layout.leftMargin: 10
            Layout.preferredHeight: 60
            Layout.rightMargin: 10
            spacing: 10
            visible: false
            z: 1

            TextField {
                id: searchField

                Layout.fillWidth: true
                placeholderText: "Search..."
                z: 1

                Keys.onReturnPressed: function (event) {
                    if (event.modifiers & Qt.ShiftModifier) {
                        contentDisplay.navigateToPreviousResult();
                        return;
                    }

                    contentDisplay.navigateToNextResult();
                }
                onTextChanged: function () {
                    contentDisplay.performSearch(text, articleSection.getText(0, articleSection.text.length));
                }
            }
            Text {
                color: "white"
                font.pixelSize: 14
                text: contentDisplay.totalResults > 0 ? contentDisplay.currentResultIndex + " of " + contentDisplay.totalResults : ""
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
            Layout.leftMargin: 10
            Layout.preferredHeight: 40
            Layout.rightMargin: 10
            spacing: 10

            Button {
                id: galleryButton

                Layout.alignment: Qt.AlignRight
                text: "Gallery"
                visible: GlobalState.currentPageTitle.length > 0

                onClicked: {
                    NavigationState.navigateToImageGallery();
                }
            }
            Button {
                id: copyHtmlButton

                Layout.alignment: Qt.AlignRight
                text: "Copy HTML"
                visible: GlobalState.currentPageTitle.length > 0 && mainContent.articleText.length > 0

                onClicked: {
                    GlobalState.copyToClipboard(mainContent.articleText);
                }
            }
            Button {
                id: sectionsButton

                Layout.alignment: Qt.AlignRight
                text: "Sections"
                visible: GlobalState.currentPageTitle.length > 0

                onClicked: {
                    mainContent.sectionsPanelVisible = !mainContent.sectionsPanelVisible;
                }
            }
        }
        ScrollView {
            id: scrollView

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

            function scrollToCursorTop() {
                // Get the cursor rectangle in the TextEdit's coordinate system
                const cursorRect = articleSection.cursorRectangle;
                // Calculate the position to scroll to
                let scrollToY = cursorRect.y - cursorRect.height;// - scrollView.height;
                // Ensure the position is within valid bounds
                scrollToY = Math.max(0, Math.min(scrollToY, scrollView.contentHeight - scrollView.height));
                // Use the ScrollBar's value property to set the scroll position
                scrollView.ScrollBar.vertical.position = scrollToY / (scrollView.contentHeight - scrollView.height);
            }

            Layout.fillHeight: true
            Layout.fillWidth: true
            ScrollBar.vertical.interactive: true
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn
            clip: true

            Column {
                id: articleDisplay

                property SystemPalette sysPalette: SystemPalette {
                }

                padding: 20
                spacing: 20
                width: scrollView.width - scrollView.effectiveScrollBarWidth - leftPadding - rightPadding

                TextEdit {
                    color: articleDisplay.sysPalette.text
                    font.bold: true
                    font.pixelSize: 20
                    readOnly: true
                    selectByMouse: true
                    selectionColor: articleDisplay.sysPalette.highlight
                    text: GlobalState.currentPageTitle
                    visible: GlobalState.currentPageTitle.length > 0
                    width: parent.width
                    wrapMode: TextEdit.Wrap
                }
                TextEdit {
                    id: articleSection

                    color: articleDisplay.sysPalette.text
                    font.pixelSize: 14
                    readOnly: true
                    selectByMouse: true
                    selectionColor: articleDisplay.sysPalette.highlight
                    text: mainContent.articleText
                    textFormat: TextEdit.RichText
                    visible: mainContent.articleText.length > 0
                    width: parent.width
                    wrapMode: TextEdit.Wrap

                    ContextMenu.menu: Menu {
                        MenuItem {
                            text: "Copy"

                            onTriggered: articleSection.copy()
                        }
                    }

                    onLinkActivated: function (link) {
                        if (link.startsWith("/wiki/")) {
                            var title = link.substring(6).replace(/_/g, " ");
                            GlobalState.loadArticleByTitle(title);
                        }
                    }
                    onLinkHovered: {}
                }
                Text {
                    color: articleDisplay.sysPalette.text
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    text: "Select an article to view its content"
                    visible: GlobalState.currentPageTitle.length === 0 && !GlobalState.isLoading
                    width: parent.width
                    wrapMode: Text.WordWrap
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: "red"
                    text: GlobalState.errorMessage
                    visible: GlobalState.errorMessage.length > 0
                    width: parent.width
                    wrapMode: Text.WordWrap
                }
            }
        }
    }
    ContentDisplayModel {
        id: contentDisplay

        onNavigateToResult: function (start, end) {
            articleSection.cursorPosition = start;
            articleSection.select(start, end);
            scrollView.scrollToCursor();
        }
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
    }

    // Sections overlay panel
    Rectangle {
        id: sectionsOverlay

        anchors.bottom: mainContent.bottom
        anchors.right: mainContent.right
        anchors.top: mainContent.top
        border.color: "#333333"
        border.width: 1
        color: "#1e1e1e"
        height: mainContent.height
        visible: mainContent.sectionsPanelVisible
        width: mainContent.width * 0.5
        x: mainContent.sectionsPanelVisible ? 0 : width
        z: 100

        // Animation for sliding in/out
        PropertyAnimation {
            id: slideAnimation

            duration: 300
            easing.type: Easing.InOutQuad
            property: "x"
            target: sectionsOverlay
            to: mainContent.sectionsPanelVisible ? 0 : width
        }

        // Close button
        Button {
            id: closeSectionsButton

            anchors.margins: 10
            anchors.right: parent.right
            anchors.top: parent.top
            height: 40
            text: "×"
            width: 40
            z: 101

            background: Rectangle {
                color: hovered ? "#333333" : "#2e2e2e"
                radius: 5
            }
            contentItem: Text {
                color: "white"
                font.pixelSize: 18
                text: "×"
            }

            onClicked: mainContent.sectionsPanelVisible = false
        }

        // Section component
        Section {
            id: sectionComponent

            anchors.bottomMargin: 10
            anchors.fill: parent
            anchors.margins: 50
            anchors.topMargin: 50

            onSectionClicked: function (section) {
                console.log("Section clicked:", section.title);
                mainContent.sectionsPanelVisible = false;
                // Scroll to section using anchor-based navigation
                var html = articleSection.text;
                var cursorPos = contentDisplay.findSectionPosition(html, section.anchor);

                if (cursorPos !== -1) {
                    articleSection.cursorPosition = cursorPos;
                    articleSection.select(cursorPos, cursorPos);
                    scrollView.scrollToCursorTop();
                } else {
                    console.log("Could not find section anchor:", section.anchor, "title:", section.title);
                }
            }
        }
    }

    // Close sections panel on escape key when visible
    Shortcut {
        sequences: ["Escape"]

        onActivated: {
            if (mainContent.sectionsPanelVisible) {
                mainContent.sectionsPanelVisible = false;
            } else if (searchBar.visible) {
                searchBar.visible = false;
            }
        }
    }
}
