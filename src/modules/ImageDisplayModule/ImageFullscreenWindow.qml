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

    onVisibleChanged: {
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
