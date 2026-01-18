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

QVector<SearchResult> GlobalState::searchResults() const {
    return m_searchResults;
}

bool GlobalState::isLoading() const {
    return m_isLoading;
}

QString GlobalState::errorMessage() const {
    return m_errorMessage;
}

void GlobalState::setSearchResults(const QVector<SearchResult> &results) {
    m_searchResults = results;
    emit searchResultsChanged();
}

void GlobalState::setCurrentPage(const Page &page) {
    m_currentPage = page;
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
