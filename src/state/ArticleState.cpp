#include "ArticleState.h"

namespace {
qsizetype pagePayloadBytes(const page &article) {
    qsizetype characters = article.title.size() + article.extract.size();
    for (const QString &value : article.imageUrls) characters += value.size();
    for (const QString &value : article.imageTitles) characters += value.size();
    for (const QString &value : article.imageDescriptions) characters += value.size();
    return characters * qsizetype(sizeof(QChar));
}
}

QPointer<ArticleState> ArticleState::m_instance;

ArticleState::ArticleState(HistoryState &history, SettingsState &settings, QObject *parent)
    : QObject(parent), m_history(&history), m_settings(&settings), m_pageClient(new WikipediaPageClient(this)) {
    Q_ASSERT(!m_instance);
    m_instance = this;
    m_pageClient->setLanguage(settings.language());
    connect(&settings, &SettingsState::languageChanged, this, [this]() {
        m_pageClient->setLanguage(m_settings->language());
        m_articleCache.clear();
        m_articleCacheRecency.clear();
        m_articleCacheBytes.clear();
        m_articleCacheByteCount = 0;
    });
    connect(&settings, &SettingsState::languageTransitionStarted, this, [this]() {
        m_pageClient->cancelPendingRequests();
        m_articleCache.clear();
        m_articleCacheRecency.clear();
        m_articleCacheBytes.clear();
        m_articleCacheByteCount = 0;
        m_currentPage = page{};
        clearErrorMessage();
        emit currentPageChanged();
        setIsLoading(false);
    });
    connect(m_pageClient, &WikipediaPageClient::pageReceived, this, &ArticleState::setCurrentPage);
    connect(m_pageClient, &WikipediaPageClient::errorOccurred, this, &ArticleState::handleArticleLoadError);
    connect(m_pageClient, &WikipediaPageClient::pageIdResolvedWithContext, this, &ArticleState::continueResolvedArticleLoad);
}

void ArticleState::setCurrentPage(const page &article) {
    const QString language = m_settings ? m_settings->language() : QString();
    m_currentPage = article;
    if (article.pageid > 0 && !article.title.isEmpty()) {
        const qsizetype articleBytes = pagePayloadBytes(article);
        if (articleBytes <= ArticleCacheByteCapacity) {
            if (m_articleCacheBytes.contains(article.pageid))
                m_articleCacheByteCount -= m_articleCacheBytes.value(article.pageid);
            m_articleCache[article.pageid] = article;
            m_articleCacheBytes[article.pageid] = articleBytes;
            m_articleCacheByteCount += articleBytes;
            m_articleCacheRecency.removeAll(article.pageid);
            m_articleCacheRecency.append(article.pageid);
        } else {
            m_articleCache.remove(article.pageid);
            m_articleCacheRecency.removeAll(article.pageid);
            if (m_articleCacheBytes.contains(article.pageid))
                m_articleCacheByteCount -= m_articleCacheBytes.take(article.pageid);
        }
        while (m_articleCache.size() > ArticleCacheCapacity || m_articleCacheByteCount > ArticleCacheByteCapacity) {
            const int leastRecentlyUsed = m_articleCacheRecency.takeFirst();
            m_articleCache.remove(leastRecentlyUsed);
            m_articleCacheByteCount -= m_articleCacheBytes.take(leastRecentlyUsed);
        }
        if (m_history)
            m_history->addToHistory(article.title, article.pageid, language);
    }
    emit currentPageChanged();
    setIsLoading(false);
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

void ArticleState::continueResolvedArticleLoad(int pageId, const WikipediaRequestContext &context) {
    if (pageId <= 0) {
        setIsLoading(false);
        return;
    }
    if (m_articleCache.contains(pageId)) {
        setCurrentPage(m_articleCache.value(pageId));
        return;
    }
    m_pageClient->getPageById(pageId, context);
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
    setErrorMessage(error);
    setIsLoading(false);
}
