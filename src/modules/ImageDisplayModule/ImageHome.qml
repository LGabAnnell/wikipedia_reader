// ImageHome.qml - Main entry point for ImageDisplayModule
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt.ImageDisplay

Item {
    id: mainContent
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0

    ImageGallery {
        anchors.fill: parent
    }
}
