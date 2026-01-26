#include "ContentDisplayModel.h"
#include <QDebug>
#include <algorithm>
#include <QFile>

ContentDisplayModel::ContentDisplayModel(QObject *parent) : QObject(parent) {
    connectSearchSignals();
}

void ContentDisplayModel::connectSearchSignals() {
    // Connect signals and slots for search functionality.
    connect(this, &ContentDisplayModel::searchRequested, this, [this](const QString &searchText, const QString &text) {
        // Perform the search and emit searchResultsAvailable with the results.
        QList<SearchIndices> indices = searchForText(searchText, text);
        m_searchResults = indices;
        m_currentResultIndex = indices.isEmpty() ? -1 : 0;
        emit searchResultsAvailable(indices);
        emit totalResultsChanged();
        emit currentResultIndexChanged();
    });
}

QList<SearchIndices> ContentDisplayModel::searchForText(const QString &searchText, const QString &text) {
    QList<SearchIndices> indices = {};
    if (searchText.isEmpty()) {
        return indices;
    }

    // Iterate through the text to find occurrences of searchText
    int startIndex = 0;
    while (startIndex >= 0) {
        startIndex = text.indexOf(searchText, startIndex, Qt::CaseInsensitive);
        if (startIndex != -1) {
            // Calculate the end index
            int endIndex = startIndex + searchText.length();

            // Add the start and end indices to the list
            indices.append(SearchIndices({ .start = startIndex, .end = endIndex}));

            // Move to the next position after the found occurrence
            startIndex += searchText.length();
        }
    }

    return indices;
}

void ContentDisplayModel::navigateToNextResult() {
    if (m_searchResults.isEmpty() || m_currentResultIndex == -1) {
        return;
    }

    m_currentResultIndex = (m_currentResultIndex + 1) % m_searchResults.size();
    SearchIndices result = m_searchResults[m_currentResultIndex];
    emit navigateToResult(result.start, result.end);
    emit currentResultIndexChanged();
}

void ContentDisplayModel::navigateToPreviousResult() {
    if (m_searchResults.isEmpty() || m_currentResultIndex == -1) {
        return;
    }

    m_currentResultIndex = (m_currentResultIndex - 1 + m_searchResults.size()) % m_searchResults.size();
    SearchIndices result = m_searchResults[m_currentResultIndex];
    emit navigateToResult(result.start, result.end);
    emit currentResultIndexChanged();
}

