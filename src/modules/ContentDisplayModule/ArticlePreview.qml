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
    property bool opensBelow: true
    readonly property real maximumPreviewHeight: 420
    readonly property real pointerGap: 12
    readonly property real spaceAbove: Math.max(0, anchorY - pointerGap - margins)
    readonly property real spaceBelow: parent
        ? Math.max(0, parent.height - margins - anchorY - pointerGap) : maximumPreviewHeight
    readonly property real availablePreviewHeight: Math.max(0, Math.min(
        opensBelow ? spaceBelow : spaceAbove,
        parent ? parent.height - 2 * margins : maximumPreviewHeight))
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
    width: Math.max(0, Math.min(340, parent ? parent.width - 2 * margins : 340))
    height: Math.min(maximumPreviewHeight, contentItem.implicitHeight, availablePreviewHeight)
    x: Math.max(margins, Math.min(anchorX, parent ? parent.width - width - margins : anchorX))
    y: Math.max(margins, Math.min(
        opensBelow ? anchorY + pointerGap : anchorY - pointerGap - height,
        parent ? parent.height - height - margins : anchorY))
    SystemPalette { id: palette }
    background: Rectangle {
        color: palette.window
        radius: 6
        border.color: palette.mid
        border.width: 1
    }
    onAboutToShow: {
        // Reserve room for loaded content before opening. Content and thumbnail
        // changes may resize the card, but must not move it across the pointer.
        opensBelow = spaceBelow >= maximumPreviewHeight
            || (spaceAbove < maximumPreviewHeight && spaceBelow >= spaceAbove)
    }
    contentItem: Item {
        implicitHeight: Math.max(140, previewColumn.implicitHeight + 28)
        clip: true
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
                Layout.fillHeight: true
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
                objectName: "articlePreviewSummaryScroll"
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

