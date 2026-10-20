import QtQuick
import QtQuick.Controls
import wikipedia_qt

Window {
    id: root
    objectName: "imageFullscreenWindow"
    title: qsTr("Image viewer")
    width: Screen.width
    height: Screen.height
    color: "black"
    modality: Qt.WindowModal

    property int visibilityGeneration: 0

    function closeAfterMouseRelease(generation: int) {
        // A queued click must not close a viewer that has since been reopened.
        if (visible && generation === visibilityGeneration)
            close();
    }

    // Load the image only while the viewer is open.
    Loader {
        anchors.fill: parent
        active: root.visible
        focus: true
        sourceComponent: ImageView {
            focus: true
            backButtonText: qsTr("Close")
            onBackRequested: root.close()

            // Claim Escape before shortcuts in the underlying article window
            // can compete for it, including when the Close button has focus.
            Keys.onShortcutOverride: event => {
                if (event.key === Qt.Key_Escape)
                    event.accepted = true;
            }
            Keys.onEscapePressed: event => {
                root.close();
                event.accepted = true;
            }
        }
    }

    // Handle mouse Back locally so closing the viewer preserves navigation.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.BackButton
        // Finish releasing the mouse grab before hiding the modal window.
        onClicked: Qt.callLater(root.closeAfterMouseRelease, root.visibilityGeneration)
    }

    onVisibleChanged: {
        ++visibilityGeneration;
        if (visible)
            requestActivate();
    }

    Connections {
        target: ImageSelectionState

        function onCurrentImageUrlChanged() {
            if (ImageSelectionState.currentImageUrl.length === 0)
                root.close();
        }
    }
}
