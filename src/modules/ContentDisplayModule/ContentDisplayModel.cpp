#include "ContentDisplayModel.h"
#include <QDebug>
#include <algorithm>
#include <QFile>
#include <QUrl>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextCharFormat>

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
    QTextDocument doc;
    doc.setHtml(html);

    // Try multiple variations of the anchor name
    QStringList targets;
    targets << anchor;
    
    // Wikipedia replaces spaces with underscores in anchors
    QString underscored = anchor;
    underscored.replace(' ', '_');
    targets << underscored;
    
    // Try with XML entities for special chars (e.g., &#8211; for en-dash, &#8212; for em-dash)
    QString xmlEscaped = anchor;
    xmlEscaped.replace(QChar(0x2013), "&#8211;");  // en-dash
    xmlEscaped.replace(QChar(0x2014), "&#8212;");  // em-dash
    targets << xmlEscaped;
    
    QString xmlEscapedUnderscored = underscored;
    xmlEscapedUnderscored.replace(QChar(0x2013), "&#8211;");
    xmlEscapedUnderscored.replace(QChar(0x2014), "&#8212;");
    targets << xmlEscapedUnderscored;

    for (QTextBlock block = doc.begin(); block != doc.end(); block = block.next()) {
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            QTextFragment frag = it.fragment();
            if (!frag.isValid()) continue;

            QTextCharFormat fmt = frag.charFormat();
            if (fmt.isAnchor()) {
                const QStringList names = fmt.anchorNames();
                for (const QString &name : names) {
                    if (targets.contains(name)) {
                        return frag.position();
                    }
                }
            }
        }
    }
    return -1;
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

