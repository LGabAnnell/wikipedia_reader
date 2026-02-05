// src/GlobalState.h
#ifndef GLOBALSTATE_H
#define GLOBALSTATE_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QQmlEngine>
#include <QPointer>
#include "wikipedia_search_client.h"
#include "wikipedia_page_client.h"
#include "wikipedia_featured_client.h"
#include "wikipedia_home_client.h"
#include "wikipedia_models.h"
#include "HistoryState.h"

class GlobalState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Singleton")

    // Expose Page properties directly
    Q_PROPERTY(QVector<search_result> searchResults READ searchResults NOTIFY searchResultsChanged)
    Q_PROPERTY(QString currentPageTitle READ currentPageTitle NOTIFY currentPageChanged)
    Q_PROPERTY(QString currentPageExtract READ currentPageExtract NOTIFY currentPageChanged)
    Q_PROPERTY(int currentPageId READ currentPageId NOTIFY currentPageChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

    // Add error message property
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    Q_INVOKABLE void loadArticleByPageId(int pageId);
    explicit GlobalState(QObject *parent = nullptr, HistoryState* historyState = nullptr);

    // Page property accessors
    QString currentPageTitle() const;
    QString currentPageExtract() const;
    int currentPageId() const;
    QVector<search_result> searchResults() const;

    bool isLoading() const;
    QString errorMessage() const;

    static QPointer<GlobalState> instance() {
        return m_instance;
    }

public slots:
    void setSearchResults(const QVector<search_result> &results);
    void setCurrentPage(const page &page);
    void setCurrentPageFromData(const QString &title, const QString &extract, const QString &url);
    void setIsLoading(bool loading);
    void setErrorMessage(const QString &message);
    void clearErrorMessage();

signals:
    void searchResultsChanged();
    void currentPageChanged();
    void isLoadingChanged();
    void errorMessageChanged();

private:
    QVector<search_result> m_searchResults;
    page m_currentPage;
    bool m_isLoading;
    QString m_errorMessage;
    static QPointer<GlobalState> m_instance;
    WikipediaSearchClient* m_searchClient;
    WikipediaPageClient* m_pageClient;
    WikipediaFeaturedClient* m_featuredClient;
    WikipediaHomeClient* m_homeClient;
    HistoryState* m_historyState;
    QMap<int, page> m_articleCache; // Cache for loaded articles

    // Helper method to check if item already exists in history
    bool itemExistsInHistory(int pageId);

private slots:
    void handleArticleLoadError(const QString &error);
};

#endif // GLOBALSTATE_H
