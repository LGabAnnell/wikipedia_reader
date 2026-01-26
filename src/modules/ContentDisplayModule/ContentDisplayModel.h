#ifndef CONTENTDISPLAYMODEL_H
#define CONTENTDISPLAYMODEL_H

#include <QObject>
#include <QString>
#include <QQmlEngine>
#include <QList>

struct SearchIndices {
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

    // Getter for currentResultIndex
    int currentResultIndex() const { return m_currentResultIndex + 1; }

    // Getter for totalResults
    int totalResults() const { return m_searchResults.size(); }

signals:
    /**
     * @brief Signal emitted when a search is performed.
     * @param searchText The text to search for.
     */
    void searchRequested(const QString &searchText, const QString &text);

    /**
     * @brief Signal emitted when search results are available.
     * @param results The list of search results.
     */
    void searchResultsAvailable(const QList<SearchIndices> &results);

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
    // Helper method to connect search signals and slots
    void connectSearchSignals();

    // Helper method to perform the search
    QList<SearchIndices> searchForText(const QString &searchText, const QString &text);

    // List of items to search through
    QStringList m_items;

    // Store the current search results
    QList<SearchIndices> m_searchResults;

    // Index of the currently selected search result
    int m_currentResultIndex = -1;
};

#endif // CONTENTDISPLAYMODEL_H

