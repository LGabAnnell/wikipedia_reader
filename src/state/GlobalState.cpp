// src/GlobalState.cpp
#include "GlobalState.h"
#include "wikipedia_search_client.h"
#include "wikipedia_page_client.h"
#include "wikipedia_featured_client.h"
#include "wikipedia_home_client.h"
#include "wikipedia_models.h"

QPointer<GlobalState> GlobalState::m_instance = nullptr; // Definition

GlobalState::GlobalState(QObject *parent, HistoryState *historyState) :
    QObject(parent),
    m_isLoading(false),
    m_isLoadingSections(false),
    m_historyState(historyState) {

    m_instance = this;
    m_searchClient = new WikipediaSearchClient(this);
    m_pageClient = new WikipediaPageClient(this);
    m_featuredClient = new WikipediaFeaturedClient(this);
    m_homeClient = new WikipediaHomeClient(this);

    // Connect WikipediaPageClient signals to GlobalState
    connect(m_pageClient, &WikipediaPageClient::pageReceived,
            this, &GlobalState::setCurrentPage);
    // NOTE: pageWithImagesReceived is intentionally NOT connected to setCurrentPage.
    // getPageWithImages uses explaintext=1, so its extract is plain text; letting it
    // overwrite the current page would strip the article's HTML formatting (and poison
    // the article cache with the plain-text version). The image gallery gets its data
    // via ImageHomeModel's own connection to pageWithImagesReceived.
    connect(m_pageClient, &WikipediaPageClient::sectionsReceived,
            this, &GlobalState::setSections);
    connect(m_pageClient, &WikipediaPageClient::errorOccurred,
            this, &GlobalState::handleArticleLoadError);

    // Connect title resolution to load by page id
    connect(m_pageClient, &WikipediaPageClient::pageIdResolved,
            this, [this](int pageid) {
                m_pageClient->getPageById(pageid);
            });

    connect(m_featuredClient, &WikipediaFeaturedClient::featuredArticleReceived,
            this, [this](const QString &title, const QString &extract, const int &pageid) {
                // Create a page object from the featured article data
                page p;
                p.title = title;
                p.extract = extract;
                p.pageid = pageid;
                p.imageUrls = QStringList(); // Initialize imageUrls as an empty list
                setCurrentPage(p);
            });

    // m_featuredClient->getFeaturedArticleOfTheDay();
}

QString GlobalState::currentPageTitle() const {
    return m_currentPage.title;
}

QString GlobalState::currentPageExtract() const {
    return m_currentPage.extract;
}

int GlobalState::currentPageId() const {
    return m_currentPage.pageid;
}

QVector<search_result> GlobalState::searchResults() const {
    return m_searchResults;
}

QStringList GlobalState::currentPageImageUrls() const {
    return m_currentPage.imageUrls;
}

QVector<section> GlobalState::currentPageSections() const {
    return m_currentPageSections;
}

bool GlobalState::isLoading() const {
    return m_isLoading;
}

bool GlobalState::isLoadingSections() const {
    return m_isLoadingSections;
}

QString GlobalState::errorMessage() const {
    return m_errorMessage;
}

QString GlobalState::currentImageUrl() const {
    return m_currentImageUrl;
}

QString GlobalState::currentImageDescription() const {
    return m_currentImageDescription;
}

void GlobalState::setSearchResults(const QVector<search_result> &results) {
    m_searchResults = results;
    emit searchResultsChanged();
}

void GlobalState::setCurrentPage(const page &page) {
    m_currentPage = page;
    setIsLoading(false);

    if (page.pageid > 0 && !page.title.isEmpty()) {
        m_historyState->addToHistory(page.title, page.pageid);
        // Cache the loaded article
        m_articleCache[page.pageid] = page;
    }

    emit currentPageChanged();
}

void GlobalState::setCurrentPageFromData(const QString &title, const QString &extract, const QString &url) {
    page newPage;
    newPage.title = title;
    newPage.extract = extract;
    newPage.pageid = 0; // Set a default pageid or fetch it if needed
    newPage.imageUrls = QStringList(); // Initialize imageUrls as an empty list
    setCurrentPage(newPage);
}

void GlobalState::setIsLoading(bool loading) {
    m_isLoading = loading;
    emit isLoadingChanged();
}

void GlobalState::setSections(const QVector<section> &sections) {
    m_currentPageSections = sections;
    setLoadingSections(false);
    emit sectionsChanged();
}

void GlobalState::setLoadingSections(bool loading) {
    m_isLoadingSections = loading;
    emit loadingSectionsChanged();
}

void GlobalState::setErrorMessage(const QString &message) {
    m_errorMessage = message;
    emit errorMessageChanged();
}

void GlobalState::clearErrorMessage() {
    m_errorMessage.clear();
    emit errorMessageChanged();
}

void GlobalState::setCurrentImageUrl(const QString &url) {
    if (m_currentImageUrl != url) {
        m_currentImageUrl = url;
        emit currentImageUrlChanged();
    }
}

void GlobalState::setCurrentImageDescription(const QString &description) {
    if (m_currentImageDescription != description) {
        m_currentImageDescription = description;
        emit currentImageDescriptionChanged();
    }
}

void GlobalState::loadArticleByPageId(int pageId) {
    if (m_pageClient) {
        // Check if the article is already in the cache
        if (m_articleCache.contains(pageId)) {
            // Use the cached article
            setCurrentPage(m_articleCache[pageId]);
            setIsLoading(false);
        } else {
            // Fetch the article from the network
            setIsLoading(true);
            clearErrorMessage();
            m_pageClient->getPageById(pageId);
        }
    }
}

void GlobalState::loadArticleByTitle(const QString &title) {
    if (m_pageClient) {
        setIsLoading(true);
        clearErrorMessage();
        m_pageClient->resolveTitleToPageId(title);
    }
}

void GlobalState::fetchSectionsForCurrentPage() {
    if (m_pageClient && !m_currentPage.title.isEmpty()) {
        setLoadingSections(true);
        clearErrorMessage();
        m_pageClient->getSections(m_currentPage.title);
    }
}

void GlobalState::copyToClipboard(const QString &text) {
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard) {
        clipboard->setText(text);
    }
}

void GlobalState::handleArticleLoadError(const QString &error) {
    setIsLoading(false);
    setErrorMessage(error);
}

void GlobalState::handleSectionsLoadError(const QString &error) {
    setLoadingSections(false);
    setErrorMessage(error);
}
