#include "ArticleState.h"

QPointer<ArticleState> ArticleState::m_instance;

ArticleState::ArticleState(HistoryState &history, SettingsState &settings, QObject *parent)
    : QObject(parent), m_history(&history), m_settings(&settings), m_pageClient(new WikipediaPageClient(this)) {
    Q_ASSERT(!m_instance);
    m_instance = this;
    m_pageClient->setLanguage(settings.language());
    connect(&settings, &SettingsState::languageChanged, this, [this]() {
        m_pageClient->setLanguage(m_settings->language());
        m_articleCache.clear();
    });
    connect(&settings, &SettingsState::languageTransitionStarted, this, [this]() {
        m_pageClient->cancelPendingRequests();
        m_articleCache.clear();
        m_currentPage = page{};
        setIsLoading(false);
        clearErrorMessage();
        emit currentPageChanged();
    });
    connect(m_pageClient, &WikipediaPageClient::pageReceived, this, &ArticleState::setCurrentPage);
    connect(m_pageClient, &WikipediaPageClient::errorOccurred, this, &ArticleState::handleArticleLoadError);
    connect(m_pageClient, &WikipediaPageClient::pageIdResolved, this, &ArticleState::continueResolvedArticleLoad);
}

void ArticleState::setCurrentPage(const page &article) {
    const QString language = m_settings ? m_settings->language() : QString();
    m_currentPage = article;
    if (article.pageid > 0 && !article.title.isEmpty()) {
        m_articleCache[article.pageid] = article;
        if (m_history)
            m_history->addToHistory(article.title, article.pageid, language);
    }
    setIsLoading(false);
    emit currentPageChanged();
}

bool ArticleState::loadArticleFromHistory(const QString &language, int pageId) {
    if (language.trimmed().isEmpty() || pageId <= 0 || !m_settings)
        return false;
    m_settings->setLanguage(language);
    loadArticleByPageId(pageId);
    return true;
}

void ArticleState::setCurrentPageFromData(const QString &title, const QString &extract, const QString &url) {
    Q_UNUSED(url);
    page article{};
    article.title = title;
    article.extract = extract;
    setCurrentPage(article);
}

void ArticleState::loadArticleByPageId(int pageId) {
    m_pageClient->cancelPendingRequests();
    clearErrorMessage();
    if (m_articleCache.contains(pageId)) {
        const page article = m_articleCache.value(pageId);
        setCurrentPage(article);
        return;
    }
    // Clear content from a previous selection before opening the new article
    // view. A failed uncached request must not leave that article visible.
    m_currentPage = page{};
    emit currentPageChanged();
    setIsLoading(true);
    m_pageClient->getPageById(pageId);
}

void ArticleState::continueResolvedArticleLoad(int pageId) {
    if (pageId <= 0) {
        setIsLoading(false);
        return;
    }
    if (m_articleCache.contains(pageId)) {
        setCurrentPage(m_articleCache.value(pageId));
        return;
    }
    m_pageClient->getPageById(pageId);
}

void ArticleState::loadArticleByTitle(const QString &title) {
    m_pageClient->cancelPendingRequests();
    clearErrorMessage();
    // Drop the current article so a newly pushed article view does not
    // briefly display the previous article while the new one loads.
    m_currentPage = page{};
    emit currentPageChanged();
    setIsLoading(true);
    m_pageClient->resolveTitleToPageId(title);
}

void ArticleState::setIsLoading(bool loading) {
    if (m_isLoading == loading)
        return;
    m_isLoading = loading;
    emit isLoadingChanged();
}

void ArticleState::setErrorMessage(const QString &message) {
    if (m_errorMessage == message)
        return;
    m_errorMessage = message;
    emit errorMessageChanged();
}

void ArticleState::clearErrorMessage() {
    setErrorMessage({});
}

void ArticleState::handleArticleLoadError(const QString &error) {
    setIsLoading(false);
    setErrorMessage(error);
}
