#include "SearchBarModel.h"

SearchBarModel::SearchBarModel(QObject *parent) : QObject(parent), m_searchState(SearchState::instance()) {
    if (!m_searchState)
        return;
    connect(m_searchState, &SearchState::isSearchingChanged, this, [this]() {
        emit isSearchingChanged(isSearching());
    });
    connect(m_searchState, &SearchState::hasCompletedSearchChanged, this, [this]() {
        emit hasCompletedSearchChanged(hasCompletedSearch());
    });
    connect(m_searchState, &SearchState::errorOccurred, this, &SearchBarModel::errorOccurred);
}

bool SearchBarModel::isSearching() const {
    return m_searchState && m_searchState->isSearching();
}

bool SearchBarModel::hasCompletedSearch() const {
    return m_searchState && m_searchState->hasCompletedSearch();
}

void SearchBarModel::setSearchText(const QString &text) {
    if (m_searchText == text)
        return;
    m_searchText = text;
    emit searchTextChanged(text);
}

void SearchBarModel::performSearch() {
    const QString query = m_searchText.trimmed();
    if (!m_searchState || query.isEmpty() || isSearching())
        return;
    emit searchRequested(query);
    m_searchState->search(query);
}

void SearchBarModel::clearSearchResults() {
    if (m_searchState)
        m_searchState->clearSearchResults();
}
