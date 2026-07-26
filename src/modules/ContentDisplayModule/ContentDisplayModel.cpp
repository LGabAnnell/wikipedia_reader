#include "ContentDisplayModel.h"
#include <QDebug>
#include <algorithm>
#include <QFile>
#include <QUrl>

ContentDisplayModel::ContentDisplayModel(QObject *parent) : QObject(parent) {
}

QList<search_indices> ContentDisplayModel::performSearch(const QString &searchText, const QString &text) {
    QList<search_indices> indices = {};
    if (searchText.isEmpty()) {
        m_searchResults = indices;
        m_currentResultIndex = -1;
        emit searchResultsAvailable(indices);
        emit totalResultsChanged();
        emit currentResultIndexChanged();
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
            indices.append(search_indices({ .start = startIndex, .end = endIndex}));

            // Move to the next position after the found occurrence
            startIndex += searchText.length();
        }
        
    }

    m_searchResults = indices;
    m_currentResultIndex = indices.isEmpty() ? -1 : 0;
    emit searchResultsAvailable(indices);
    emit totalResultsChanged();
    emit currentResultIndexChanged();

    return indices;
}

int ContentDisplayModel::findSectionPosition(const QString &html, const QString &anchor) {
    // Search for the anchor in id attributes: id="anchor" or id='anchor'
    QString pattern1 = "id=\"" + anchor + "\"";
    QString pattern2 = "id='" + anchor + "'";
    
    qDebug() << "findSectionPosition: searching for pattern1:" << pattern1 << "pattern2:" << pattern2;
    qDebug() << "HTML contains Political_career?" << html.contains("Political_career");
    qDebug() << "HTML contains id=\"Political_career\"?" << html.contains("id=\"Political_career\"");
    
    int pos = html.indexOf(pattern1);
    if (pos == -1) {
        pos = html.indexOf(pattern2);
    }
    
    // If not found, try with spaces replaced by underscores (Wikipedia does this)
    if (pos == -1) {
        QString underscoreAnchor = anchor;
        underscoreAnchor.replace(' ', '_');
        pattern1 = "id=\"" + underscoreAnchor + "\"";
        pattern2 = "id='" + underscoreAnchor + "'";
        pos = html.indexOf(pattern1);
        if (pos == -1) {
            pos = html.indexOf(pattern2);
        }
    }
    
    // If still not found, try with XML entities for special chars (e.g., &#8211; for en-dash)
    if (pos == -1) {
        QString xmlEscaped = anchor;
        xmlEscaped.replace(QChar(0x2013), "&#8211;");  // en-dash
        xmlEscaped.replace(QChar(0x2014), "&#8212;");  // em-dash
        pattern1 = "id=\"" + xmlEscaped + "\"";
        pattern2 = "id='" + xmlEscaped + "'";
        pos = html.indexOf(pattern1);
        if (pos == -1) {
            pos = html.indexOf(pattern2);
        }
    }
    
    if (pos == -1) {
        return -1;
    }
    
    // Find the end of the opening tag (the '>' character)
    int tagEnd = html.indexOf('>', pos);
    if (tagEnd == -1) {
        return -1;
    }
    
    // The text position is after the '>' character, so start counting from there
    // We need to count all non-tag characters in the HTML up to the '>' of the opening tag
    // This gives us the text position where the section content starts
    int textPos = 0;
    bool inTag = false;
    for (int i = 0; i <= tagEnd; i++) {
        if (html[i] == '<') {
            inTag = true;
        } else if (html[i] == '>') {
            inTag = false;
        } else if (!inTag) {
            // This character is outside a tag, so it's part of the text content
            textPos++;
        }
    }
    
    return textPos;
}

void ContentDisplayModel::navigateToNextResult() {
    if (m_searchResults.isEmpty() || m_currentResultIndex == -1) {
        return;
    }

    m_currentResultIndex = (m_currentResultIndex + 1) % m_searchResults.size();
    search_indices result = m_searchResults[m_currentResultIndex];
    emit navigateToResult(result.start, result.end);
    emit currentResultIndexChanged();
}

void ContentDisplayModel::navigateToPreviousResult() {
    if (m_searchResults.isEmpty() || m_currentResultIndex == -1) {
        return;
    }

    m_currentResultIndex = (m_currentResultIndex - 1 + m_searchResults.size()) % m_searchResults.size();
    search_indices result = m_searchResults[m_currentResultIndex];
    emit navigateToResult(result.start, result.end);
    emit currentResultIndexChanged();
}

