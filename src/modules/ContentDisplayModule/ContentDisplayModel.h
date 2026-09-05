#ifndef CONTENTDISPLAYMODEL_H
#define CONTENTDISPLAYMODEL_H

#include <QQmlEngine>
#include <QList>
#include <QVector>

struct search_indices {
    Q_GADGET
        QML_ELEMENT
        Q_PROPERTY(qsizetype start MEMBER start);
        Q_PROPERTY(qsizetype end MEMBER end);

public:
    qsizetype start;
    qsizetype end;
};

class ContentDisplayModel : public QObject
{
    Q_OBJECT
        QML_ELEMENT

    Q_PROPERTY(int currentResultIndex READ currentResultIndex NOTIFY currentResultIndexChanged)
    Q_PROPERTY(int totalResults READ totalResults NOTIFY totalResultsChanged)

public:
    explicit ContentDisplayModel(QObject *parent = nullptr);

    // Method to navigate to the next search result
    Q_INVOKABLE void navigateToNextResult();

    // Method to navigate to the previous search result
    Q_INVOKABLE void navigateToPreviousResult();

    // Method to perform search and return results
    Q_INVOKABLE QList<search_indices> performSearch(const QString &searchText, const QString &text);

    // Method to find section position by anchor
    Q_INVOKABLE int findSectionPosition(const QString &html, const QString &anchor);

    // Pre-compute character positions for all sections in the article HTML.
    // sections is a list of objects with an "anchor" property.
    Q_INVOKABLE void updateSectionPositions(const QString &html, const QVariantList &sections);

    // Binary-search the pre-computed positions for the last section whose
    // character position <= charPosition. Returns the section index or -1.
    Q_INVOKABLE int findSectionAtPosition(int charPosition);

    // Getter for currentResultIndex
    int currentResultIndex() const { return m_currentResultIndex + 1; }

    // Getter for totalResults
    int totalResults() const { return m_searchResults.size(); }

signals:
    /**
     * @brief Signal emitted when a search is performed.
     * @param searchText The text to search for.
     * @param text The text to search within.
     */
    void searchRequested(const QString &searchText, const QString &text);

    /**
     * @brief Signal emitted when search results are available.
     * @param results The list of search results.
     */
    void searchResultsAvailable(const QList<search_indices> &results);

    /**
     * @brief Signal emitted when navigating to a specific search result.
     * @param start The start index of the search result.
     * @param end The end index of the search result.
     */
    void navigateToResult(qsizetype start, qsizetype end);

    // Signal emitted when currentResultIndex changes
    void currentResultIndexChanged();

    // Signal emitted when totalResults changes
    void totalResultsChanged();

private:
    // List of items to search through
    QStringList m_items;

    // Store the current search results
    QList<search_indices> m_searchResults;

    // Index of the currently selected search result
    int m_currentResultIndex = -1;

    // Sorted (charPosition, sectionIndex) pairs for scroll tracking
    QVector<QPair<int, int>> m_sectionPositions;
};

#endif // CONTENTDISPLAYMODEL_H

