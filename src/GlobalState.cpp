// src/GlobalState.cpp
#include "GlobalState.h"
#include "wikipedia_client/wikipedia_client.h"

GlobalState::GlobalState(QObject *parent) : QObject(parent), m_isLoading(false) {}

QVector<Wikipedia::SearchResult> GlobalState::searchResults() const {
    return m_searchResults;
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

bool GlobalState::isLoading() const {
    return m_isLoading;
}

QString GlobalState::errorMessage() const {
    return m_errorMessage;
}

void GlobalState::setSearchResults(const QVector<Wikipedia::SearchResult> &results) {
    m_searchResults = results;
    emit searchResultsChanged();
}

void GlobalState::setCurrentPage(const Wikipedia::Page &page) {
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
