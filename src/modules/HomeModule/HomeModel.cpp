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

    // Connect new signals for News, On This Day, and Did You Know
    connect(m_wikipediaClient, &WikipediaClient::newsItemsReceived,
            this, &HomeModel::handleNewsItemsReceived);
    connect(m_wikipediaClient, &WikipediaClient::onThisDayEventsReceived,
            this, &HomeModel::handleOnThisDayEventsReceived);
    connect(m_wikipediaClient, &WikipediaClient::didYouKnowItemsReceived,
            this, &HomeModel::handleDidYouKnowItemsReceived);
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

// QML-compatible getter (QVariantList)
QVariantList HomeModel::newsItems() const {
    QVariantList variantList;
    for (const auto &item : m_newsItems) {
        QVariantMap newsItem;
        newsItem["title"] = item.title;
        newsItem["imageUrl"] = item.imageUrl;
        newsItem["description"] = item.description;
        newsItem["url"] = QString("https://en.wikipedia.org/wiki?curid=%1").arg(item.url);
        variantList.append(newsItem);
    }
    return variantList;
}

// QML-compatible getter (QVariantList)
QVariantList HomeModel::onThisDayEvents() const {
    QVariantList variantList;
    for (const auto &event : m_onThisDayEvents) {
        QVariantMap eventItem;
        eventItem["year"] = event.year;
        eventItem["event"] = event.event;
        eventItem["url"] = event.url;
        variantList.append(eventItem);
    }
    return variantList;
}

// QML-compatible getter (QVariantList)
QVariantList HomeModel::didYouKnowItems() const {
    QVariantList variantList;
    for (const auto &item : m_didYouKnowItems) {
        QVariantMap dykItem;
        dykItem["text"] = item.text;
        dykItem["url"] = item.url;
        variantList.append(dykItem);
    }
    return variantList;
}

// Strongly-typed getter for internal use
QVector<news_item> HomeModel::getNewsItemsTyped() const {
    return m_newsItems;
}

// Strongly-typed getter for internal use
QVector<on_this_day_event> HomeModel::getOnThisDayEventsTyped() const {
    return m_onThisDayEvents;
}

// Strongly-typed getter for internal use
QVector<did_you_know_item> HomeModel::getDidYouKnowItemsTyped() const {
    return m_didYouKnowItems;
}

void HomeModel::fetchHomeData() {
    // Fetch featured article of the day
    m_wikipediaClient->getFeaturedArticleOfTheDay();

    // Fetch real data from Wikipedia
    m_wikipediaClient->getNewsItems();
    m_wikipediaClient->getOnThisDayEvents();
    m_wikipediaClient->getDidYouKnowItems();
}

void HomeModel::handleFeaturedArticleReceived(const QString &title, const QString &extract, const int &pageid) {
    m_featuredArticleTitle = title;
    m_featuredArticleExtract = extract;
    m_featuredArticleUrl = QString("https://en.wikipedia.org/wiki/%1").arg(m_featuredArticleTitle.replace(' ', '_'));
    
    // Use a placeholder image initially
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
    // In a production app, you might want to implement fallback to sample data here
}

void HomeModel::handleNewsItemsReceived(const QVector<news_item> &items) {
    m_newsItems = items;
    emit dataUpdated();
}

void HomeModel::handleOnThisDayEventsReceived(const QVector<on_this_day_event> &events) {
    m_onThisDayEvents = events;
    emit dataUpdated();
}

void HomeModel::handleDidYouKnowItemsReceived(const QVector<did_you_know_item> &items) {
    m_didYouKnowItems = items;
    emit dataUpdated();
}
