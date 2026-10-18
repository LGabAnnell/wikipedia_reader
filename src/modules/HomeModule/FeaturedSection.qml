import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import wikipedia_qt

ColumnLayout {
    id: root
    objectName: "featuredSection"

    property string title: ""
    property string extract: ""
    property string imageUrl: ""
    property int pageId: 0
    property int status: 0
    property string errorMessage: ""

    signal retryRequested()

    spacing: 10
    Layout.margins: 20

    Label {
        text: "Featured article"
        font.pixelSize: 20
        font.bold: true
    }

    HomeFeedback {
        width: parent.width
        sectionName: "featured"
        status: root.status
        errorMessage: root.errorMessage
        emptyMessage: "No featured article available."
        onRetryRequested: root.retryRequested()
    }

    RowLayout {
        visible: root.status === 2
        Layout.fillWidth: true

        Image {
            objectName: "featuredArticleImage"
            visible: root.width > 480 && status === Image.Ready
            source: root.imageUrl
            sourceSize: Qt.size(180, 120)
            fillMode: Image.PreserveAspectCrop
            Layout.preferredWidth: visible ? 180 : 0
            Layout.preferredHeight: visible ? 120 : 0
        }

        ColumnLayout {
            Layout.fillWidth: true

            Label {
                objectName: "featuredTitle"
                text: root.title
                textFormat: Text.PlainText
                font.pixelSize: 18
                font.bold: true
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            Label {
                text: root.extract
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            Button {
                objectName: "featuredReadArticleButton"
                text: "Read article"
                onClicked: {
                    if (root.pageId > 0) {
                        ArticleState.loadArticleByPageId(root.pageId)
                        NavigationState.navigateToContent()
                    }
                }
            }
        }
    }
}
