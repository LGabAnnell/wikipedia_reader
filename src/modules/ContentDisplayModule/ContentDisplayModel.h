#ifndef CONTENTDISPLAY_H
#define CONTENTDISPLAY_H

#include <QObject>
#include <QString>
#include <QQmlEngine>

class ContentDisplayModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    ContentDisplayModel(QObject *parent = nullptr);

    /**
     * @brief Adds search functionality to the ContentDisplay QML component.
     *
     * This method integrates a search bar and search results into the ContentDisplay QML component.
     * It connects the search functionality to the backend logic for fetching and displaying search results.
     */
    Q_INVOKABLE void addSearchFunctionality();

signals:
    /**
     * @brief Signal emitted when a search is performed.
     * @param searchText The text to search for.
     */
    void searchRequested(const QString &searchText);

    /**
     * @brief Signal emitted when search results are available.
     * @param results The list of search results.
     */
    void searchResultsAvailable(const QStringList &results);

private:
    // Helper method to set up search bar UI
    void setupSearchBar();

    // Helper method to set up search results display
    void setupSearchResults();

    // Helper method to connect search signals and slots
    void connectSearchSignals();

};

#endif // CONTENTDISPLAY_H