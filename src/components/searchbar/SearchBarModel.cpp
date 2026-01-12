#include "SearchBarModel.h"

#include "wikipedia_client/wikipedia_client.h"

SearchBarModel *SearchBarModel::m_instance = nullptr;

SearchBarModel::SearchBarModel(QObject *parent) : QObject(parent) {
    m_isSearching = false;
    m_searchText = "hello";

    if (!m_instance) {
        m_instance = this;
    }
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
    }
}

const SearchBarModel *SearchBarModel::instance() {
    return m_instance;
}