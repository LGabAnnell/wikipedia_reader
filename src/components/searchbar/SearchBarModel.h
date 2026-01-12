#ifndef SEARCHBAR_H
#define SEARCHBAR_H

#include <QObject>
#include <QString>
#include <QQmlEngine>

/**
 * @brief The SearchBar class provides the backend logic for the SearchBar QML component.
 *
 * This class handles the search functionality, including processing search queries
 * and emitting signals when the search text changes.
 */
class SearchBarModel : public QObject {
    Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
        Q_PROPERTY(bool isSearching READ isSearching NOTIFY isSearchingChanged)

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

    /**
     * @brief Gets the singleton instance of the SearchBarModel.
     * @return The singleton instance of the SearchBarModel.
     */
    static const SearchBarModel *instance();

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

public slots:
    /**
     * @brief Initiates a search with the current search text.
     */
    void performSearch();

private:
    QString m_searchText; ///< The current search text.
    bool m_isSearching;   ///< Indicates whether a search is currently in progress.
    static SearchBarModel *m_instance;
};

#endif // SEARCHBAR_H