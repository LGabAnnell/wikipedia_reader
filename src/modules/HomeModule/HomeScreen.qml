// src/modules/HomeModule/HomeScreen.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import wikipedia_qt
import wikipedia_qt.Home

ScrollView {
    id: root
    anchors.fill: parent
    clip: true
    
    ColumnLayout {
        id: mainColumn
        width: parent.width
        spacing: 20
        
        // Featured article section
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 300
            
            RowLayout {
                anchors.fill: parent
                spacing: 20
                
                // Featured article image
                Image {
                    id: featuredImage
                    Layout.preferredWidth: 250
                    Layout.preferredHeight: 250
                    Layout.alignment: Qt.AlignTop
                    fillMode: Image.PreserveAspectFit
                    source: HomeModel.featuredArticleImageUrl
                    clip: true
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            // Navigate to the featured article
                            GlobalState.currentPageTitle = HomeModel.featuredArticleTitle
                            GlobalState.currentPageExtract = HomeModel.featuredArticleExtract
                            NavigationState.navigateToView(Constants.contentView())
                        }
                    }
                }
                
                // Featured article content
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    
                    Label {
                        Layout.fillWidth: true
                        text: "From today's featured article"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#666666"
                    }
                    
                    Label {
                        Layout.fillWidth: true
                        text: HomeModel.featuredArticleTitle
                        font.pixelSize: 24
                        font.bold: true
                        wrapMode: Text.WordWrap
                        color: "#002bb8"
                        
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                GlobalState.currentPageTitle = HomeModel.featuredArticleTitle
                                GlobalState.currentPageExtract = HomeModel.featuredArticleExtract
                                NavigationState.navigateToView(Constants.contentView())
                            }
                        }
                    }
                    
                    Label {
                        Layout.fillWidth: true
                        text: HomeModel.featuredArticleExtract
                        font.pixelSize: 14
                        wrapMode: Text.WordWrap
                        lineHeight: 1.3
                    }
                    
                    Label {
                        Layout.fillWidth: true
                        text: "Full article..."
                        font.pixelSize: 14
                        color: "#002bb8"
                        font.italic: true
                        
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                GlobalState.currentPageTitle = HomeModel.featuredArticleTitle
                                GlobalState.currentPageExtract = HomeModel.featuredArticleExtract
                                NavigationState.navigateToView(Constants.contentView())
                            }
                        }
                    }
                }
            }
        }
        
        // Horizontal divider
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#a2a9b1"
        }
        
        // In the news section
        Item {
            Layout.fillWidth: true
            
            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                
                Label {
                    Layout.fillWidth: true
                    text: "In the news"
                    font.pixelSize: 20
                    font.bold: true
                }
                
                // News items list
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 15
                    
                    Repeater {
                        model: HomeModel.newsItems
                        
                        Item {
                            Layout.fillWidth: true
                            Layout.preferredHeight: newsItemLayout.implicitHeight
                            
                            RowLayout {
                                id: newsItemLayout
                                anchors.fill: parent
                                spacing: 15
                                
                                // News image if available
                                Image {
                                    visible: modelData.imageUrl
                                    width: 80
                                    height: 60
                                    fillMode: Image.PreserveAspectFit
                                    source: modelData.imageUrl
                                    clip: true
                                }
                                
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    
                                    Label {
                                        Layout.fillWidth: true
                                        text: "• " + modelData.title
                                        font.pixelSize: 14
                                        font.bold: true
                                        color: "#002bb8"
                                        wrapMode: Text.WordWrap
                                        
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: {
                                                GlobalState.currentPageTitle = modelData.title
                                                GlobalState.currentPageExtract = modelData.description
                                                NavigationState.navigateToView(Constants.contentView())
                                            }
                                        }
                                    }
                                    
                                    Label {
                                        Layout.fillWidth: true
                                        text: modelData.description
                                        font.pixelSize: 14
                                        wrapMode: Text.WordWrap
                                    }
                                }
                            }
                        }
                    }
                }
                
                Label {
                    Layout.fillWidth: true
                    text: "Ongoing: Iranian protests • Russo-Ukrainian war (timeline) • Sudanese civil war (timeline) • U.S. immigration enforcement in Minneapolis (protests)"
                    font.pixelSize: 12
                    color: "#666666"
                    wrapMode: Text.WordWrap
                }
                
                Label {
                    Layout.fillWidth: true
                    text: "Recent deaths: Mark Tully • Michael F. Adams • William Foege • Kevin Johnson • Marie Bashir • Nandana Gunathilake"
                    font.pixelSize: 12
                    color: "#666666"
                    wrapMode: Text.WordWrap
                }
                
                Label {
                    Layout.fillWidth: true
                    text: "More current events • Nominate an article"
                    font.pixelSize: 12
                    color: "#002bb8"
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            // Navigate to current events page
                        }
                    }
                }
            }
        }
        
        // Horizontal divider
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#a2a9b1"
        }
        
        // On this day section
        Item {
            Layout.fillWidth: true
            
            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                
                Label {
                    Layout.fillWidth: true
                    text: "On this day"
                    font.pixelSize: 20
                    font.bold: true
                }
                
                // On this day events
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    
                    Repeater {
                        model: HomeModel.onThisDayEvents
                        
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 15
                            
                            Label {
                                text: modelData.year
                                font.pixelSize: 14
                                font.bold: true
                                Layout.alignment: Qt.AlignTop
                                width: 60
                            }
                            
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 5
                                
                                Label {
                                    Layout.fillWidth: true
                                    text: "• " + modelData.event
                                    font.pixelSize: 14
                                    wrapMode: Text.WordWrap
                                    
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            GlobalState.currentPageTitle = "On this day: " + modelData.year
                                            GlobalState.currentPageExtract = modelData.event
                                            NavigationState.navigateToView(Constants.contentView())
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                
                // Star chart image
                Image {
                    Layout.alignment: Qt.AlignHCenter
                    source: "qrc:/images/star_chart.jpg"
                    width: 150
                    height: 150
                    fillMode: Image.PreserveAspectFit
                }
                
                Label {
                    Layout.fillWidth: true
                    text: "Johann Nikolaus von Hontheim (b. 1701) • Sasaki Tōichi (b. 1886) • Giuseppe Verdi (d. 1901) • Victoria Ocampo (d. 1979)"
                    font.pixelSize: 12
                    color: "#666666"
                    wrapMode: Text.WordWrap
                }
                
                Label {
                    Layout.fillWidth: true
                    text: "More anniversaries: January 26 • January 27 • January 28"
                    font.pixelSize: 12
                    color: "#002bb8"
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            // Navigate to anniversaries page
                        }
                    }
                }
            }
        }
        
        // Horizontal divider
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#a2a9b1"
        }
        
        // Did you know section
        Item {
            Layout.fillWidth: true
            
            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                
                Label {
                    Layout.fillWidth: true
                    text: "Did you know ..."
                    font.pixelSize: 20
                    font.bold: true
                }
                
                // Did you know items
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    
                    Repeater {
                        model: HomeModel.didYouKnowItems
                        
                        Label {
                            Layout.fillWidth: true
                            text: "• " + modelData.text
                            font.pixelSize: 14
                            wrapMode: Text.WordWrap
                            
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    GlobalState.currentPageTitle = "Did you know: " + modelData.text
                                    GlobalState.currentPageExtract = ""
                                    NavigationState.navigateToView(Constants.contentView())
                                }
                            }
                        }
                    }
                }
                
                Label {
                    Layout.fillWidth: true
                    text: "Archive • By email • More featured articles • About"
                    font.pixelSize: 12
                    color: "#002bb8"
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            // Navigate to archive page
                        }
                    }
                }
            }
        }
    }
}