import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts


import wikipedia_qt.Home

Item {
    id: root
    Layout.fillWidth: true

    ScrollView {
        id: scrollView
        objectName: "homeScrollView"
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.vertical.interactive: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        property SystemPalette sysPalette: SystemPalette {}

        HomeModel {
            id: homeModel
            objectName: "homeModel"
        }

        ColumnLayout {
            id: mainColumn
            width: scrollView.availableWidth
            spacing: 20
            FeaturedSection {
                Layout.fillWidth: true
                title: homeModel.featuredArticleTitle
                extract: homeModel.featuredArticleExtract
                imageUrl: homeModel.featuredArticleImageUrl
                pageId: homeModel.featuredArticlePageId
                status: homeModel.featuredStatus
                errorMessage: homeModel.featuredError
                onRetryRequested: homeModel.retrySection(HomeModel.Featured)
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: scrollView.sysPalette.text }
            // News section
            NewsSection {
                id: newsSection
                model: homeModel.newsItems
                status: homeModel.newsStatus
                errorMessage: homeModel.newsError
                onRetryRequested: homeModel.retrySection(HomeModel.News)
                Layout.fillWidth: true
            }

            // Add a white separator line
            Rectangle {
                Layout.preferredWidth: scrollView.width
                height: 1
                color: scrollView.sysPalette.text
            }

            OnThisDaySection {
                id: onThisDaySection
                model: homeModel.onThisDayEvents
                status: homeModel.onThisDayStatus
                errorMessage: homeModel.onThisDayError
                onRetryRequested: homeModel.retrySection(HomeModel.OnThisDay)
                Layout.fillWidth: true
            }

            Rectangle {
                Layout.preferredWidth: scrollView.width
                height: 1
                color: scrollView.sysPalette.text
            }

            DidYouKnowSection {
                id: didYouKnowSection
                model: homeModel.didYouKnowItems
                status: homeModel.didYouKnowStatus
                errorMessage: homeModel.didYouKnowError
                onRetryRequested: homeModel.retrySection(HomeModel.DidYouKnow)
                Layout.fillWidth: true
            }
        }
    }

    Component.onCompleted: function () {
        homeModel.fetchHomeData();
    }
}
