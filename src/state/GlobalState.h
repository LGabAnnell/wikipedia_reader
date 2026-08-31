// src/GlobalState.h
#ifndef GLOBALSTATE_H
#define GLOBALSTATE_H

#include "HistoryState.h"
#include "wikipedia_featured_client.h"
#include "wikipedia_home_client.h"
#include "wikipedia_models.h"
#include "wikipedia_page_client.h"
#include "wikipedia_search_client.h"
#include <QClipboard>
#include <QGuiApplication>
#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QString>
#include <QVector>

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
    Q_PROPERTY(QVector<section> currentPageSections READ currentPageSections NOTIFY sectionsChanged)
    Q_PROPERTY(bool isLoadingSections READ isLoadingSections NOTIFY loadingSectionsChanged)

    // Add error message property
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

    // Currently selected image URL (for the fullscreen image view)
    Q_PROPERTY(QString currentImageUrl READ currentImageUrl WRITE setCurrentImageUrl NOTIFY currentImageUrlChanged)

    // Description/caption for the currently selected image (fullscreen image view overlay)
    Q_PROPERTY(QString currentImageDescription READ currentImageDescription WRITE setCurrentImageDescription NOTIFY
                   currentImageDescriptionChanged)

  public:
    Q_INVOKABLE void loadArticleByPageId(int pageId);
    Q_INVOKABLE void loadArticleByTitle(const QString &title);
    Q_INVOKABLE void copyToClipboard(const QString &text);
    Q_INVOKABLE void fetchSectionsForCurrentPage();
    explicit GlobalState(QObject *parent = nullptr, HistoryState *historyState = nullptr);

    // Page property accessors
    QString currentPageTitle() const;
    QString currentPageExtract() const;
    int currentPageId() const;
    QVector<search_result> searchResults() const;
    QStringList currentPageImageUrls() const;
    QVector<section> currentPageSections() const;

    bool isLoading() const;
    bool isLoadingSections() const;
    QString errorMessage() const;
    QString currentImageUrl() const;
    QString currentImageDescription() const;

    // Accessor for WikipediaPageClient
    WikipediaPageClient *pageClient() const { return m_pageClient; }

    static QPointer<GlobalState> instance() { return m_instance; }

  public slots:
    void setSearchResults(const QVector<search_result> &results);
    void setCurrentPage(const page &page);
    void setCurrentPageFromData(const QString &title, const QString &extract, const QString &url);
    void setIsLoading(bool loading);
    void setSections(const QVector<section> &sections);
    void setLoadingSections(bool loading);
    void setErrorMessage(const QString &message);
    void clearErrorMessage();
    void setCurrentImageUrl(const QString &url);
    void setCurrentImageDescription(const QString &description);

  signals:
    void searchResultsChanged();
    void currentPageChanged();
    void isLoadingChanged();
    void sectionsChanged();
    void loadingSectionsChanged();
    void errorMessageChanged();
    void currentImageUrlChanged();
    void currentImageDescriptionChanged();

  private:
    QVector<search_result> m_searchResults;
    page m_currentPage;
    QVector<section> m_currentPageSections;
    bool m_isLoading;
    bool m_isLoadingSections;
    QString m_errorMessage;
    QString m_currentImageUrl;
    QString m_currentImageDescription;
    static QPointer<GlobalState> m_instance;
    WikipediaSearchClient *m_searchClient;
    WikipediaPageClient *m_pageClient;
    WikipediaFeaturedClient *m_featuredClient;
    WikipediaHomeClient *m_homeClient;
    HistoryState *m_historyState;
    QMap<int, page> m_articleCache; // Cache for loaded articles

    // Helper method to check if item already exists in history
    bool itemExistsInHistory(int pageId);

  private slots:
    void handleArticleLoadError(const QString &error);
    void handleSectionsLoadError(const QString &error);
};

#endif // GLOBALSTATE_H
