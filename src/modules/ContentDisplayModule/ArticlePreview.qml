import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: preview
    objectName: "articlePreviewPopup"
    property var previewModel
    property real anchorX: 0
    property real anchorY: 0
    property bool pointerInside: false
    signal pointerExited()
    signal pointerEntered()
    parent: Overlay.overlay
    popupType: Popup.Item
    modal: false
    dim: false
    focus: false
    padding: 0
    margins: 12
    closePolicy: Popup.NoAutoClose
    width: Math.min(340, parent ? parent.width - 24 : 340)
    height: Math.min(420, contentItem.implicitHeight, parent ? parent.height - 24 : 420)
    SystemPalette { id: palette }
    background: Rectangle {
        color: palette.window
        radius: 6
        border.color: palette.mid
        border.width: 1
    }
    function positionPopup() {
        if (!parent)
            return
        const below = anchorY + height + 12 <= parent.height
        x = Math.max(12, Math.min(anchorX, parent.width - width - 12))
        y = below ? anchorY + 12 : Math.max(12, anchorY - height - 12)
    }
    onAboutToShow: positionPopup()
    onHeightChanged: {
        if (visible)
            positionPopup()
    }
    contentItem: Item {
        implicitHeight: Math.max(140, previewColumn.implicitHeight + 28)
        HoverHandler {
            onHoveredChanged: {
                preview.pointerInside = hovered
                if (hovered)
                    preview.pointerEntered()
                else if (preview.visible)
                    preview.pointerExited()
            }
        }
        ColumnLayout {
            id: previewColumn
            anchors.fill: parent
            anchors.margins: 14
            spacing: 8
            Text {
                objectName: "articlePreviewTitle"
                Layout.fillWidth: true
                text: preview.previewModel ? preview.previewModel.title : ""
                font.bold: true
                font.pixelSize: 16
                maximumLineCount: 2
                elide: Text.ElideRight
                color: palette.text
                wrapMode: Text.WordWrap
            }
            Image {
                objectName: "articlePreviewThumbnail"
                Layout.fillWidth: true
                Layout.preferredHeight: source.toString().length ? 140 : 0
                Layout.maximumHeight: 140
                Layout.minimumHeight: 0
                source: preview.previewModel ? preview.previewModel.thumbnailUrl : ""
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                visible: status === Image.Ready
            }
            BusyIndicator {
                objectName: "articlePreviewLoading"
                Layout.alignment: Qt.AlignHCenter
                running: preview.previewModel && preview.previewModel.status === ArticlePreviewModel.Loading
                visible: running
            }
            Text {
                objectName: "articlePreviewUnavailable"
                Layout.fillWidth: true
                text: qsTr("Preview unavailable")
                visible: preview.previewModel && preview.previewModel.status === ArticlePreviewModel.Unavailable
                color: palette.text
                wrapMode: Text.WordWrap
            }
            ScrollView {
                id: summaryScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 0
                Layout.preferredHeight: Math.min(previewText.implicitHeight, 170)
                contentWidth: availableWidth
                contentHeight: previewText.implicitHeight
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                clip: true
                visible: preview.previewModel && preview.previewModel.status === ArticlePreviewModel.Ready
                Text {
                    id: previewText
                    objectName: "articlePreviewSummary"
                    width: Math.max(0, preview.width - 28)
                    textFormat: Text.RichText
                    text: preview.previewModel ? preview.previewModel.summaryHtml : ""
                    color: palette.text
                    wrapMode: Text.Wrap
                }
            }
            Button {
                objectName: "articlePreviewOpenButton"
                Layout.fillWidth: true
                Layout.minimumHeight: 36
                Layout.preferredHeight: 36
                text: qsTr("Open article")
                visible: preview.previewModel && preview.previewModel.status !== ArticlePreviewModel.Idle
                onClicked: preview.openRequested()
            }
        }
    }
    signal openRequested()
}

