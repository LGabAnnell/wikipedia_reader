// src/GlobalState.cpp
#include "GlobalState.h"
#include "wikipedia_client.h"

QPointer<GlobalState> GlobalState::m_instance = nullptr; // Definition

GlobalState::GlobalState(QObject *parent) : QObject(parent), m_isLoading(false) {
    m_instance = this;
}

QVector<SearchResult> GlobalState::searchResults() const {
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
