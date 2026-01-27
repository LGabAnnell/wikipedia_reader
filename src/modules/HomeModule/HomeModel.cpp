// src/modules/HomeModule/HomeModel.cpp
#include "HomeModel.h"
#include <QDate>
#include <QDebug>
#include <QString>
#include "wikipedia_client.h"

HomeModel::HomeModel(QObject *parent) : QObject(parent) {
    // Initialize Wikipedia client
    m_wikipediaClient = new WikipediaClient(this);

    // Connect signals from WikipediaClient
    connect(m_wikipediaClient, &WikipediaClient::featuredArticleReceived,
            this, &HomeModel::handleFeaturedArticleReceived);
    connect(m_wikipediaClient, &WikipediaClient::pageWithImagesReceived,
            this, &HomeModel::handlePageWithImagesReceived);
    connect(m_wikipediaClient, &WikipediaClient::errorOccurred,
            this, &HomeModel::handleError);

    // Fetch initial data
    fetchHomeData();
}

HomeModel* HomeModel::create(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(engine);
    Q_UNUSED(scriptEngine);
    
    // Create a singleton instance
    static HomeModel* instance = new HomeModel();
    return instance;
}

QString HomeModel::featuredArticleTitle() const {
    return m_featuredArticleTitle;
}

QString HomeModel::featuredArticleExtract() const {
    return m_featuredArticleExtract;
}

QString HomeModel::featuredArticleImageUrl() const {
    return m_featuredArticleImageUrl;
}

QString HomeModel::featuredArticleUrl() const {
    return m_featuredArticleUrl;
}

QVariantList HomeModel::newsItems() const {
    return m_newsItems;
}

QVariantList HomeModel::onThisDayEvents() const {
    return m_onThisDayEvents;
}

QVariantList HomeModel::didYouKnowItems() const {
    return m_didYouKnowItems;
}

void HomeModel::fetchHomeData() {
    // Fetch featured article of the day
    m_wikipediaClient->getFeaturedArticleOfTheDay();

    // TODO: Implement methods for fetching news items and on this day events
    // For now, we'll keep the sample data for these sections
    initializeSampleNewsData();
    initializeSampleOnThisDayData();
    initializeSampleDidYouKnowData();

    // Notify UI that data has been updated
    emit dataUpdated();
}

void HomeModel::handleFeaturedArticleReceived(const QString &title, const QString &extract, const int &pageid) {
    m_featuredArticleTitle = title;
    m_featuredArticleExtract = extract;
    m_featuredArticleUrl = QString("https://en.wikipedia.org/wiki/%1").arg(m_featuredArticleTitle.replace(' ', '_'));

    // For now, use a placeholder image - in a real implementation, we'd fetch the actual image
    m_featuredArticleImageUrl = "qrc:/images/featured_article_placeholder.jpg";

    // Fetch the page with images to get the actual image URL
    m_wikipediaClient->getPageWithImages(pageid);

    emit featuredArticleUpdated();
}

void HomeModel::handlePageWithImagesReceived(const page &page) {
    if (page.title == m_featuredArticleTitle && !page.imageUrls.isEmpty()) {
        // Use the first image URL if available
        m_featuredArticleImageUrl = page.imageUrls.first();
        emit featuredArticleImageUpdated();
    }
}

void HomeModel::handleError(const QString &error) {
    qWarning() << "Wikipedia API error:" << error;
    // Fallback to sample data if API fails
    if (m_featuredArticleTitle.isEmpty()) {
        m_featuredArticleTitle = error;
        emit dataUpdated();
    }
}

void HomeModel::initializeSampleNewsData() {
    // News items - using the example from the screenshot
    m_newsItems = {
        QVariantMap{
            {"title", "Iliana Iotova becomes the first female president of Bulgaria"},
            {"imageUrl", "qrc:/images/iliana_iotova.jpg"},
            {"description", "following the resignation of Rumen Radev."},
            {"url", "Iliana_Iotova"}
        },
        QVariantMap{
            {"title", "Vietnam's Communist Party Congress re-elects Tô Lâm as General Secretary"},
            {"description", "the most powerful position in the one-party state."},
            {"url", "Vietnam_Communist_Party_Congress_2024"}
        },
        QVariantMap{
            {"title", "Italian fashion designer Valentino dies at the age of 93"},
            {"description", "founder of his eponymous fashion house."},
            {"url", "Valentino_Garavani"}
        },
        QVariantMap{
            {"title", "Wildfires in Chile leave more than 20 people dead"},
            {"description", "Ongoing disasters in South America."},
            {"url", "Chile_wildfires_2024"}
        }
    };
}

void HomeModel::initializeSampleOnThisDayData() {
    // On this day events - using the example from the screenshot
    QDate today = QDate::currentDate();
    m_onThisDayEvents = {
        QVariantMap{
            {"year", 1343},
            {"event", "Clement VI issued the papal bull Unigenitus, justifying the power of the pope and the use of indulgences."},
            {"url", "Unigenitus"}
        },
        QVariantMap{
            {"year", 1726},
            {"event", "J.S. Bach led the first performance of Alles nur nach Gottes Willen, BWV 72, concluding his third Christmas season in Leipzig on the Third Sunday after Epiphany."},
            {"url", "Alles_nur_nach_Gottes_Willen_BWV_72"}
        },
        QVariantMap{
            {"year", 1785},
            {"event", "The University of Georgia, the oldest state-chartered public university in the United States, was founded."},
            {"url", "University_of_Georgia"}
        },
        QVariantMap{
            {"year", 1996},
            {"event", "Mahamane Ousmane, the first democratically elected president of Niger, was deposed by Colonel Ibrahim Baré Maïnassara in a military coup d'état."},
            {"url", "Mahamane_Ousmane"}
        },
        QVariantMap{
            {"year", 2011},
            {"event", "Astronomers documented H1504+65, a white dwarf in Ursa Minor (chart pictured) with the hottest surface temperature known at the time at 200,000 kelvins (360,000 °F)."},
            {"url", "H1504+65"}
        }
    };
}

void HomeModel::initializeSampleDidYouKnowData() {
    // Did you know items - using the example from the screenshot
    m_didYouKnowItems = {
        QVariantMap{
            {"text", "that a lake at the John Deere World Headquarters (pictured) is used for air conditioning?"},
            {"url", "John_Deere_World_Headquarters"}
        },
        QVariantMap{
            {"text", "that a 16th-century painting is based on a sultan's description of himself as a god?"},
            {"url", "Sultan_as_god_painting"}
        },
        QVariantMap{
            {"text", "that Idaho's Ridgevue High School collaborated with a local museum to select their mascot, which honors the P-40 Warhawk World War II fighter plane?"},
            {"url", "Ridgevue_High_School_mascot"}
        },
        QVariantMap{
            {"text", "that volunteers at Takahanga Marae served more than 10,000 meals in the week following the 2016 Kaikōura earthquake?"},
            {"url", "Takahanga_Marae"}
        },
        QVariantMap{
            {"text", "that Mary Scott, Countess of Deloraine once pulled off the headdress of a court rival and declared Look at her, do you see how bald she is?"},
            {"url", "Mary_Scott_Countess_of_Deloraine"}
        }
    };
}