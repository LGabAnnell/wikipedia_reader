import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt
import wikipedia_qt.Home

Item {
    id: root
    Layout.fillWidth: true

    ScrollView {
        id: scrollView
        anchors.fill: parent
        clip: true
        ScrollBar.vertical.interactive: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        property SystemPalette sysPalette: SystemPalette {}

        HomeModel {
            id: homeModel
        }

        ColumnLayout {
            id: mainColumn
            spacing: 20
            // News section
            NewsSection {
                id: newsSection
                model: homeModel.newsItems
                Layout.preferredWidth: scrollView.width - 40
            }

            // Add a white separator line
            Rectangle {
                Layout.preferredWidth: scrollView.width
                height: 1
                color: scrollView.sysPalette.text
            }

            OnThisDaySection {
                id: onThisDaySection
                model: homeModel.onThisDayItems
                Layout.preferredWidth: scrollView.width - 40
            }

            Rectangle {
                Layout.preferredWidth: scrollView.width
                height: 1
                color: scrollView.sysPalette.text
            }

            DidYouKnowSection {
                id: didYouKnowSection
                model: homeModel.didYouKnowItems
                Layout.preferredWidth: scrollView.width - 40
            }
        }
    }

    Component.onCompleted: function () {
        homeModel.fetchHomeData();
    }
}
