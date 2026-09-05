#include "ContentDisplayModel.h"
#include "wikipedia_models.h"
#include <algorithm>
#include <QFile>
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

void ContentDisplayModel::updateSectionPositions(const QString &html, const QVariantList &sections) {
    m_sectionPositions.clear();

    if (html.isEmpty() || sections.isEmpty())
        return;

    QTextDocument doc;
    doc.setHtml(html);

    for (int i = 0; i < sections.size(); ++i) {
        const QVariant &sectionVar = sections[i];
        section sec = qvariant_cast<section>(sectionVar);
        if (sec.anchor.isEmpty())
            continue;

        // Build target variations (same logic as findSectionPosition)
        QStringList targets;
        targets << sec.anchor;

        QString underscored = sec.anchor;
        underscored.replace(' ', '_');
        targets << underscored;

        QString xmlEscaped = sec.anchor;
        xmlEscaped.replace(QChar(0x2013), "&#8211;");
        xmlEscaped.replace(QChar(0x2014), "&#8212;");
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
                            m_sectionPositions.append({frag.position(), sec.index});
                            break;
                        }
                    }
                }
            }
        }
    }

    // Sort by character position so binary search works
    std::sort(m_sectionPositions.begin(), m_sectionPositions.end(),
              [](const QPair<int, int> &a, const QPair<int, int> &b) {
                  return a.first < b.first;
              });
}

int ContentDisplayModel::findSectionAtPosition(int charPosition) {
    if (m_sectionPositions.isEmpty())
        return -1;

    // If before the first section anchor, no section is active
    if (charPosition < m_sectionPositions.first().first)
        return -1;

    // Binary search for the last entry whose position <= charPosition
    int lo = 0;
    int hi = m_sectionPositions.size() - 1;
    int result = -1;

    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (m_sectionPositions[mid].first <= charPosition) {
            result = m_sectionPositions[mid].second;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }

    return result;
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

