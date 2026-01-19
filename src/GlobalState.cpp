// src/GlobalState.cpp
#include "GlobalState.h"
#include "wikipedia_client.h"

QPointer<GlobalState> GlobalState::m_instance = nullptr; // Definition

GlobalState::GlobalState(QObject *parent) : QObject(parent), m_isLoading(false) {
    m_instance = this;
    m_wikipediaClient = new WikipediaClient(this);

    // Connect WikipediaClient signals to GlobalState
    connect(m_wikipediaClient, &WikipediaClient::pageReceived,
            this, &GlobalState::setCurrentPage);
    connect(m_wikipediaClient, &WikipediaClient::errorOccurred,
            this, &GlobalState::handleArticleLoadError);
    connect(m_wikipediaClient, &WikipediaClient::featuredArticleReceived,
            this, [this](const QString &title, const QString &extract, const int &pageid) {
                // Create a page object from the featured article data
                page p;
                p.title = title;
                p.extract = extract;
                p.pageid = pageid;
                setCurrentPage(p);
            });

    m_wikipediaClient->getFeaturedArticleOfTheDay();
}

QString GlobalState::currentPageTitle() const {
    return m_currentPage.title;
}

QString GlobalState::currentPageExtract() const {
    return m_currentPage.extract;
}

int GlobalState::currentPageId() const {
    return m_currentPage.pageid;
}

QVector<search_result> GlobalState::searchResults() const {
    return m_searchResults;
}

bool GlobalState::isLoading() const {
    return m_isLoading;
}

QString GlobalState::errorMessage() const {
    return m_errorMessage;
}

void GlobalState::setSearchResults(const QVector<search_result> &results) {
    m_searchResults = results;
    emit searchResultsChanged();
}

void GlobalState::setCurrentPage(const page &page) {
    m_currentPage = page;
    setIsLoading(false); // Stop loading when page is received
    emit currentPageChanged();
}

void GlobalState::setIsLoading(bool loading) {
    m_isLoading = loading;
    emit isLoadingChanged();
}

void GlobalState::setErrorMessage(const QString &message) {
    m_errorMessage = message;
    emit errorMessageChanged();
}

void GlobalState::clearErrorMessage() {
    m_errorMessage.clear();
    emit errorMessageChanged();
}

void GlobalState::loadArticleByPageId(int pageId) {
    if (m_wikipediaClient) {
        setIsLoading(true);
        clearErrorMessage();
        m_wikipediaClient->getPageById(pageId);
    }
}

void GlobalState::handleArticleLoadError(const QString &error) {
    setIsLoading(false);
    setErrorMessage(error);
}
