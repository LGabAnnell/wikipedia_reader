#ifndef SEARCHSTATE_H
#define SEARCHSTATE_H

#include "SettingsState.h"
#include "wikipedia_search_client.h"
#include <QPointer>
#include <QQmlEngine>

/** @brief Shared article search lifecycle, independent of article loading. */
class SearchState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Singleton")
    Q_PROPERTY(QVector<search_result> searchResults READ searchResults NOTIFY searchResultsChanged)
    Q_PROPERTY(bool isSearching READ isSearching NOTIFY isSearchingChanged)
    Q_PROPERTY(bool hasCompletedSearch READ hasCompletedSearch NOTIFY hasCompletedSearchChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

  public:
    explicit SearchState(SettingsState &settings, QObject *parent = nullptr);
    static QPointer<SearchState> instance() { return m_instance; }
    QVector<search_result> searchResults() const { return m_results; }
    bool isSearching() const { return m_isSearching; }
    bool hasCompletedSearch() const { return m_hasCompletedSearch; }
    QString errorMessage() const { return m_errorMessage; }
    /** @brief Submit nonblank text unless a search is already pending. */
    Q_INVOKABLE void search(const QString &text);
    Q_INVOKABLE void clearSearchResults();
    /** @brief Reset idle search feedback when starting a fresh search view. */
    Q_INVOKABLE void reset();

  public slots:
    void setSearchResults(const QVector<search_result> &results);

  signals:
    void searchResultsChanged();
    void isSearchingChanged();
    void hasCompletedSearchChanged();
    void errorMessageChanged();
    void errorOccurred(const QString &error);

  private:
    static QPointer<SearchState> m_instance;
    QPointer<SettingsState> m_settings;
    WikipediaSearchClient *m_searchClient;
    QVector<search_result> m_results;
    bool m_isSearching = false;
    bool m_hasCompletedSearch = false;
    QString m_errorMessage;
    void setIsSearching(bool searching);
    void setHasCompletedSearch(bool completed);
    void setErrorMessage(const QString &error);
};

#endif // SEARCHSTATE_H
