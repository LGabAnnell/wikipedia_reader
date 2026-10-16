#include "HomeModel.h"
#include <QDate>
#include <QDebug>
#include <QUrl>
#include <QString>
#include "SettingsState.h"
#include "wikipedia_featured_client.h"
#include "wikipedia_home_client.h"
#include "wikipedia_page_client.h"
#include "wikipedia_models.h"

HomeModel::HomeModel(QObject *parent) : QObject(parent) {
    // Initialize Wikipedia clients
    m_featuredClient = new WikipediaFeaturedClient(this);
    m_homeClient = new WikipediaHomeClient(this);
    m_pageClient = new WikipediaPageClient(this);

    m_settings = SettingsState::instance();
    if (m_settings) {
        m_featuredClient->setLanguage(m_settings->language());
        m_homeClient->setLanguage(m_settings->language());
        m_pageClient->setLanguage(m_settings->language());

        connect(m_settings, &SettingsState::languageChanged, this, [this]() {
            QString lang = m_settings->language();
            m_featuredClient->setLanguage(lang);
            m_homeClient->setLanguage(lang);
            m_pageClient->setLanguage(lang);
            fetchHomeData();
        });
        connect(m_settings, &SettingsState::languageTransitionStarted, this, [this]() {
            m_featuredClient->cancelPendingRequests();
            m_homeClient->cancelPendingRequests();
            m_pageClient->cancelPendingRequests();
            m_featuredArticleTitle.clear();
            m_featuredArticleExtract.clear();
            m_featuredArticleUrl.clear();
            m_featuredArticleImageUrl.clear();
            m_newsItems.clear();
            m_onThisDayEvents.clear();
            m_didYouKnowItems.clear();
            emit featuredArticleUpdated();
            emit featuredArticleImageUpdated();
            emit dataUpdated();
        });
    }

    // Connect signals from WikipediaFeaturedClient
    connect(m_featuredClient, &WikipediaFeaturedClient::featuredArticleReceived,
            this, &HomeModel::handleFeaturedArticleReceived);
    connect(m_pageClient, &WikipediaPageClient::pageWithImagesReceived,
            this, &HomeModel::handlePageWithImagesReceived);
    connect(m_featuredClient, &WikipediaFeaturedClient::errorOccurred,
            this, &HomeModel::handleError);

    // Connect new signals for News, On This Day, and Did You Know
    connect(m_homeClient, &WikipediaHomeClient::newsItemsReceived,
            this, &HomeModel::handleNewsItemsReceived);
    connect(m_homeClient, &WikipediaHomeClient::onThisDayEventsReceived,
            this, &HomeModel::handleOnThisDayEventsReceived);
    connect(m_homeClient, &WikipediaHomeClient::didYouKnowItemsReceived,
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

void HomeModel::fetchHomeData() {
    // All independent feeds in this refresh share the same generation.
    m_featuredClient->cancelPendingRequests();
    m_homeClient->cancelPendingRequests();
    m_pageClient->cancelPendingRequests();
    // Fetch featured article of the day
    m_featuredClient->getFeaturedArticleOfTheDay();

    // Fetch real data from Wikipedia
    m_homeClient->getNewsItems();

    // Pass the current month and day to getOnThisDayEvents
    QDate currentDate = QDate::currentDate();
    m_homeClient->getOnThisDayEvents(currentDate.month(), currentDate.day());

    m_homeClient->getDidYouKnowItems();
}

void HomeModel::handleFeaturedArticleReceived(const QString &title, const QString &extract, const int &pageid) {
    m_featuredArticleTitle = title;
    m_featuredArticleExtract = extract;
    QString pageTitle = title;
    const QString pageSlug = QString::fromLatin1(QUrl::toPercentEncoding(pageTitle.replace(' ', '_')));
    m_featuredArticleUrl = QString("https://%1.wikipedia.org/wiki/%2")
                               .arg(m_settings ? m_settings->language() : "en", pageSlug);
    
    // Use a placeholder image initially
    m_featuredArticleImageUrl = "qrc:/images/featured_article_placeholder.jpg";

    // Fetch the page with images to get the actual image URL
    m_pageClient->getPageWithImages(pageid);

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

