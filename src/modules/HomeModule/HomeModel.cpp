#include "HomeModel.h"
#include <QDate>
#include <QUrl>

HomeModel::HomeModel(QObject *parent) : QObject(parent) {
    m_featuredClient = new WikipediaFeaturedClient(this);
    m_homeClient = new WikipediaHomeClient(this);
    m_settings = SettingsState::instance();
    if (m_settings) {
        m_featuredClient->setLanguage(m_settings->language());
        m_homeClient->setLanguage(m_settings->language());
        connect(m_settings, &SettingsState::languageTransitionStarted, this, [this]() {
            m_featuredClient->cancelPendingRequests();
            m_homeClient->cancelPendingRequests();
            for (int i = 0; i < 4; ++i) {
                clearSection(static_cast<Section>(i));
                setStatus(static_cast<Section>(i), Idle);
            }
        });
        connect(m_settings, &SettingsState::languageChanged, this, [this]() {
            m_featuredClient->setLanguage(m_settings->language());
            m_homeClient->setLanguage(m_settings->language());
            fetchHomeData();
        });
    }
    connect(m_featuredClient, &WikipediaFeaturedClient::featuredArticleReceived, this,
            [this](const QString &title, const QString &extract, int pageid, const QString &thumbnail) {
                m_featuredArticleTitle = QString(title).replace('_', ' ');
                m_featuredArticleExtract = extract;
                m_featuredArticlePageId = pageid;
                m_featuredArticleImageUrl = thumbnail;
                QString slug = QString::fromLatin1(QUrl::toPercentEncoding(title));
                m_featuredArticleUrl = QString("https://%1.wikipedia.org/wiki/%2")
                                           .arg(m_settings ? m_settings->language() : "en", slug.replace("%20", "_"));
                setStatus(Featured, Ready);
                emit featuredArticleUpdated();
            });
    connect(m_featuredClient, &WikipediaFeaturedClient::errorOccurred, this, [this](const QString &error) {
        if (m_featuredClient) {
            setStatus(Featured, Error, error);
        }
    });
    connect(m_featuredClient, &WikipediaFeaturedClient::featuredArticleUnavailable, this,
            [this]() { setStatus(Featured, Empty); });
    connect(m_homeClient, &WikipediaHomeClient::sectionError, this, &HomeModel::handleHomeError);
    connect(m_homeClient, &WikipediaHomeClient::newsItemsReceived, this, [this](const auto &items) {
        m_newsItems = items;
        setStatus(News, items.isEmpty() ? Empty : Ready);
        emit dataUpdated();
    });
    connect(m_homeClient, &WikipediaHomeClient::onThisDayEventsReceived, this, [this](const auto &items) {
        m_onThisDayEvents = items;
        setStatus(OnThisDay, items.isEmpty() ? Empty : Ready);
        emit dataUpdated();
    });
    connect(m_homeClient, &WikipediaHomeClient::didYouKnowItemsReceived, this, [this](const auto &items) {
        m_didYouKnowItems = items;
        setStatus(DidYouKnow, items.isEmpty() ? Empty : Ready);
        emit dataUpdated();
    });
}

void HomeModel::setStatus(Section section, Status status, const QString &error) {
    const int i = static_cast<int>(section);
    m_status[i] = status;
    m_errors[i] = error;
    emit statusChanged();
}
void HomeModel::clearSection(Section s) {
    switch (s) {
    case Featured:
        m_featuredArticleTitle.clear();
        m_featuredArticleExtract.clear();
        m_featuredArticleImageUrl.clear();
        m_featuredArticleUrl.clear();
        m_featuredArticlePageId = 0;
        emit featuredArticleUpdated();
        break;
    case News:
        m_newsItems.clear();
        emit dataUpdated();
        break;
    case OnThisDay:
        m_onThisDayEvents.clear();
        emit dataUpdated();
        break;
    case DidYouKnow:
        m_didYouKnowItems.clear();
        emit dataUpdated();
        break;
    }
}
void HomeModel::fetchHomeData() {
    m_featuredClient->cancelPendingRequests();
    m_homeClient->cancelPendingRequests();
    for (int i = 0; i < 4; ++i) {
        const auto s = static_cast<Section>(i);
        clearSection(s);
        setStatus(s, Loading);
    }
    for (int i = 0; i < 4; ++i)
        loadSection(static_cast<Section>(i));
}
void HomeModel::retrySection(Section s) {
    if (s < Featured || s > DidYouKnow || m_status[static_cast<int>(s)] == Loading)
        return;
    clearSection(s);
    setStatus(s, Loading);
    loadSection(s);
}
void HomeModel::loadSection(Section s) {
    switch (s) {
    case Featured:
        m_featuredClient->getFeaturedArticleOfTheDay();
        break;
    case News:
        m_homeClient->getNewsItems();
        break;
    case OnThisDay: {
        const auto d = QDate::currentDate();
        m_homeClient->getOnThisDayEvents(d.month(), d.day());
        break;
    }
    case DidYouKnow:
        m_homeClient->getDidYouKnowItems();
        break;
    }
}
void HomeModel::handleHomeError(WikipediaHomeClient::Section section, const QString &error) {
    setStatus(section == WikipediaHomeClient::News        ? News
              : section == WikipediaHomeClient::OnThisDay ? OnThisDay
                                                          : DidYouKnow,
              Error, error);
}
