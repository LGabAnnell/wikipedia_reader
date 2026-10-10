#ifndef SEARCHBARMODEL_H
#define SEARCHBARMODEL_H

#include "SearchState.h"
#include <QObject>
#include <QPointer>
#include <QQmlEngine>

/** @brief Search text input and submission adapter for the shared search state. */
class SearchBarModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(bool isSearching READ isSearching NOTIFY isSearchingChanged)
    Q_PROPERTY(bool hasCompletedSearch READ hasCompletedSearch NOTIFY hasCompletedSearchChanged)

  public:
    explicit SearchBarModel(QObject *parent = nullptr);
    QString searchText() const { return m_searchText; }
    bool isSearching() const;
    bool hasCompletedSearch() const;
    void setSearchText(const QString &text);
    Q_INVOKABLE void clearSearchResults();

  public slots:
    /** @brief Submit the trimmed input if it is nonblank and search is idle. */
    void performSearch();

  signals:
    void searchTextChanged(const QString &text);
    void searchRequested(const QString &query);
    void isSearchingChanged(bool isSearching);
    void hasCompletedSearchChanged(bool hasCompletedSearch);
    void errorOccurred(const QString &error);

  private:
    QString m_searchText;
    QPointer<SearchState> m_searchState;
};

#endif // SEARCHBARMODEL_H
