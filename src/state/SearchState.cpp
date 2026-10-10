#include "SearchState.h"

QPointer<SearchState> SearchState::m_instance;

SearchState::SearchState(SettingsState &settings, QObject *parent)
    : QObject(parent), m_settings(&settings), m_searchClient(new WikipediaSearchClient(this)) {
    Q_ASSERT(!m_instance);
    m_instance = this;
    m_searchClient->setLanguage(settings.language());
    connect(&settings, &SettingsState::languageChanged, this, [this]() {
        m_searchClient->setLanguage(m_settings->language());
    });
    connect(m_searchClient, &WikipediaSearchClient::searchCompleted, this,
            [this](const QVector<search_result> &results) {
                setSearchResults(results);
                setHasCompletedSearch(true);
                setIsSearching(false);
            });
    connect(m_searchClient, &WikipediaSearchClient::errorOccurred, this, [this](const QString &error) {
        setErrorMessage(error);
        setHasCompletedSearch(false);
        setIsSearching(false);
        emit errorOccurred(error);
    });
}

void SearchState::search(const QString &text) {
    const QString query = text.trimmed();
    if (query.isEmpty() || m_isSearching)
        return;
    setIsSearching(true);
    setHasCompletedSearch(false);
    clearSearchResults();
    setErrorMessage({});
    m_searchClient->search(query);
}

void SearchState::clearSearchResults() {
    setSearchResults({});
}

void SearchState::reset() {
    if (m_isSearching)
        return;
    clearSearchResults();
    setHasCompletedSearch(false);
    setErrorMessage({});
}

void SearchState::setSearchResults(const QVector<search_result> &results) {
    m_results = results;
    emit searchResultsChanged();
}

void SearchState::setIsSearching(bool searching) {
    if (m_isSearching == searching)
        return;
    m_isSearching = searching;
    emit isSearchingChanged();
}

void SearchState::setHasCompletedSearch(bool completed) {
    if (m_hasCompletedSearch == completed)
        return;
    m_hasCompletedSearch = completed;
    emit hasCompletedSearchChanged();
}

void SearchState::setErrorMessage(const QString &error) {
    if (m_errorMessage == error)
        return;
    m_errorMessage = error;
    emit errorMessageChanged();
}
