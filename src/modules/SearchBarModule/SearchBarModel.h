#ifndef SEARCHBAR_H
#define SEARCHBAR_H

#include <QObject>
#include <QString>
#include <QQmlEngine>
#include <QPointer>
#include "GlobalState.h"
#include "HistoryState.h"

class WikipediaSearchClient; // Forward declaration

/**
 * @brief The SearchBar class provides the backend logic for the SearchBar QML component.
 *
 * This class handles the search functionality, including processing search queries
 * and emitting signals when the search text changes.
 */
class SearchBarModel : public QObject {
    Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
        Q_PROPERTY(bool isSearching READ isSearching NOTIFY isSearchingChanged)
        Q_PROPERTY(QPointer<GlobalState> globalState READ globalState)

public:

    explicit SearchBarModel(QObject *parent = nullptr);

    /**
     * @brief Gets the current search text.
     * @return The current search text.
     */
    QString searchText() const;

    /**
     * @brief Checks if a search is currently in progress.
     * @return True if a search is in progress, false otherwise.
     */
    bool isSearching() const; // Add this line to the public section of the SearchBarModel class definition in SearchBarModel.h

    /**
     * @brief Sets the search text.
     * @param text The new search text.
     */
    void setSearchText(const QString &text);

    QPointer<GlobalState> globalState() {
        return m_globalState;
    }

    // Add method to clear search results
    Q_INVOKABLE void clearSearchResults();

signals:
    /**
     * @brief Emitted when the search text changes.
     * @param text The new search text.
     */
    void searchTextChanged(const QString &text);

    /**
     * @brief Emitted when a search is initiated.
     * @param query The search query.
     */
    void searchRequested(const QString &query);
    /**
     * @brief Emitted when the search status changes.
     * @param isSearching True if a search is in progress, false otherwise.
     */
    void isSearchingChanged(bool isSearching);

    // Add error signal
    void errorOccurred(const QString &error);

    void globalStateChanged(QPointer<GlobalState> globalState); // Add this line to the signals section of the SearchBarModel class definition in SearchBarModel.h if it's not already include

public slots:
    /**
     * @brief Initiates a search with the current search text.
     */
    void performSearch();
private slots:
    void handleError(const QString &error);

private:
    QString m_searchText; ///< The current search text.
    bool m_isSearching;   ///< Indicates whether a search is currently in progress.
    QPointer<GlobalState> m_globalState; ///< The GlobalState instance.
    QPointer<WikipediaSearchClient> m_searchClient; ///< The Wikipedia search client instance.
};

#endif // SEARCHBAR_H