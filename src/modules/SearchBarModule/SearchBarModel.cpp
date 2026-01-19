// src/components/searchbar/SearchBarModel.cpp
#include "SearchBarModel.h"
#include "wikipedia_client.h"
#include "GlobalState.h"

SearchBarModel::SearchBarModel(QObject *parent) : QObject(parent) {
    m_isSearching = false;
    m_searchText = "";
    m_wikipediaClient = new WikipediaClient(this);
    m_globalState = GlobalState::instance();
    connect(m_wikipediaClient, &WikipediaClient::searchCompleted,
                m_globalState, [this] (const QVector<search_result> results) {
                    m_isSearching = false;
                    emit isSearchingChanged(m_isSearching);
                    m_globalState->setSearchResults(results);
                });
    connect(m_wikipediaClient, &WikipediaClient::pageReceived,
            m_globalState, &GlobalState::setCurrentPage);
    connect(m_wikipediaClient, &WikipediaClient::errorOccurred,
            this, &SearchBarModel::handleError);
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

void SearchBarModel::performSearch() {
    if (!m_searchText.isEmpty()) {
        m_isSearching = true;
        emit isSearchingChanged(m_isSearching);
        emit searchRequested(m_searchText);

        if (m_globalState) {
            m_globalState->setIsLoading(true);
            // Clear previous search results and errors
            m_globalState->setSearchResults({});
            m_globalState->clearErrorMessage();
        }

        if (m_wikipediaClient) {
            m_wikipediaClient->search(m_searchText);
        }
    }
}

void SearchBarModel::handleError(const QString &error) {
    qWarning() << "Search error:" << error;
    if (m_globalState) {
        m_globalState->setIsLoading(false);
        m_globalState->setErrorMessage(error);
    }
    m_isSearching = false;
    emit isSearchingChanged(m_isSearching);
    emit errorOccurred(error);
}

void SearchBarModel::clearSearchResults() {
    if (m_globalState) {
        m_globalState->setSearchResults({});
    }
}

