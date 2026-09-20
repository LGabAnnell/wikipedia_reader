// src/components/searchbar/SearchBarModel.cpp
#include "SearchBarModel.h"
#include "wikipedia_search_client.h"
#include "wikipedia_page_client.h"
#include "wikipedia_models.h"
#include "GlobalState.h"

SearchBarModel::SearchBarModel(QObject *parent) : QObject(parent) {
    m_isSearching = false;
    m_searchText = "";
    m_searchClient = new WikipediaSearchClient(this);
    m_globalState = GlobalState::instance();
    if (m_globalState) {
        m_searchClient->setLanguage(m_globalState->language());
    }
    connect(m_searchClient, &WikipediaSearchClient::searchCompleted,
                m_globalState, [this] (const QVector<search_result> results) {
                    m_isSearching = false;
                    emit isSearchingChanged(m_isSearching);
                    m_globalState->setSearchResults(results);
                    m_globalState->setIsLoading(false);
                    m_hasCompletedSearch = true;
                    emit hasCompletedSearchChanged(m_hasCompletedSearch);
                });
    connect(m_searchClient, &WikipediaSearchClient::errorOccurred,
            this, &SearchBarModel::handleError);
    connect(m_globalState, &GlobalState::languageChanged, this, [this]() {
        m_searchClient->setLanguage(m_globalState->language());
    });
}

QString SearchBarModel::searchText() const {
    return m_searchText;
}

void SearchBarModel::setSearchText(const QString &text) {
    if (m_searchText != text) {
        m_searchText = text;
        emit searchTextChanged(m_searchText);
    }
}

bool SearchBarModel::isSearching() const {
    return m_isSearching;
}

bool SearchBarModel::hasCompletedSearch() const {
    return m_hasCompletedSearch;
}

void SearchBarModel::performSearch() {
    const QString query = m_searchText.trimmed();
    if (!query.isEmpty() && !m_isSearching) {
        m_isSearching = true;
        emit isSearchingChanged(m_isSearching);
        if (m_hasCompletedSearch) {
            m_hasCompletedSearch = false;
            emit hasCompletedSearchChanged(m_hasCompletedSearch);
        }
        emit searchRequested(query);

        if (m_globalState) {
            m_globalState->setIsLoading(true);
            // Clear previous search results and errors
            m_globalState->setSearchResults({});
            m_globalState->clearErrorMessage();
        }

        if (m_searchClient) {
            m_searchClient->search(query);
        }
    }
}

void SearchBarModel::handleError(const QString &error) {
    if (m_globalState) {
        m_globalState->setIsLoading(false);
        m_globalState->setErrorMessage(error);
    }
    m_isSearching = false;
    emit isSearchingChanged(m_isSearching);
    if (m_hasCompletedSearch) {
        m_hasCompletedSearch = false;
        emit hasCompletedSearchChanged(m_hasCompletedSearch);
    }
    emit errorOccurred(error);
}

void SearchBarModel::clearSearchResults() {
    if (m_globalState) {
        m_globalState->setSearchResults({});
    }
}
