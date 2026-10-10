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
    connect(m_pageClient, &WikipediaPageClient::pageReceived, this, &ArticleState::setCurrentPage);
    connect(m_pageClient, &WikipediaPageClient::errorOccurred, this, &ArticleState::handleArticleLoadError);
    connect(m_pageClient, &WikipediaPageClient::pageIdResolved, this, &ArticleState::loadArticleByPageId);
}

void ArticleState::setCurrentPage(const page &article) {
    m_currentPage = article;
    setIsLoading(false);
    if (article.pageid > 0 && !article.title.isEmpty()) {
        if (m_history)
            m_history->addToHistory(article.title, article.pageid);
        m_articleCache[article.pageid] = article;
    }
    emit currentPageChanged();
}

void ArticleState::setCurrentPageFromData(const QString &title, const QString &extract, const QString &url) {
    Q_UNUSED(url);
    page article{};
    article.title = title;
    article.extract = extract;
    setCurrentPage(article);
}

void ArticleState::loadArticleByPageId(int pageId) {
    clearErrorMessage();
    if (m_articleCache.contains(pageId)) {
        const page article = m_articleCache.value(pageId);
        setCurrentPage(article);
        return;
    }
    setIsLoading(true);
    m_pageClient->getPageById(pageId);
}

void ArticleState::loadArticleByTitle(const QString &title) {
    clearErrorMessage();
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
