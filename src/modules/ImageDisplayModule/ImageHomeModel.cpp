#include "ImageHomeModel.h"
#include "wikipedia_page_client.h"

ImageHomeModel::ImageHomeModel(QObject *parent)
    : QObject(parent), m_article(ArticleState::instance()), m_settings(SettingsState::instance()),
      m_pageClient(new WikipediaPageClient(this)), m_currentPageId(-1) {
    if (m_settings) {
        m_pageClient->setLanguage(m_settings->language());
        connect(m_settings, &SettingsState::languageChanged, this, [this]() {
            m_pageClient->setLanguage(m_settings->language());
        });
    }
    if (m_article)
        connect(m_article, &ArticleState::currentPageChanged, this, &ImageHomeModel::handlePageWithImagesReceived);
    connect(m_pageClient, &WikipediaPageClient::pageWithImagesReceived, this, [this](const page &article) {
        m_imageUrls = article.imageUrls;
        m_imageDescriptions = article.imageDescriptions;
        m_articleTitle = article.title;
        m_currentPageId = article.pageid;
        m_isLoading = false;
        emit imageUrlsChanged();
        emit imageDescriptionsChanged();
        emit articleTitleChanged();
        emit currentPageIdChanged();
        emit loadingChanged();
    });
    connect(m_pageClient, &WikipediaPageClient::errorOccurred, this, [this](const QString &error) {
        m_errorMessage = error;
        m_isLoading = false;
        emit errorChanged();
        emit loadingChanged();
    });
}

QStringList ImageHomeModel::imageUrls() const { return m_imageUrls; }
QStringList ImageHomeModel::imageDescriptions() const { return m_imageDescriptions; }
QString ImageHomeModel::articleTitle() const { return m_articleTitle; }
int ImageHomeModel::currentPageId() const { return m_currentPageId; }

void ImageHomeModel::loadImagesForCurrentPage() {
    if (m_article)
        loadImagesForPage(m_article->currentPageId());
}

void ImageHomeModel::loadImagesForPage(int pageId) {
    if (pageId <= 0)
        return;
    m_currentPageId = pageId;
    m_imageUrls.clear();
    m_imageDescriptions.clear();
    m_isLoading = true;
    m_errorMessage.clear();
    emit currentPageIdChanged();
    emit imageUrlsChanged();
    emit imageDescriptionsChanged();
    emit loadingChanged();
    emit errorChanged();
    m_pageClient->getPageWithImages(pageId);
}

void ImageHomeModel::handlePageWithImagesReceived() {
    if (!m_article)
        return;
    const page article = m_article->currentPage();
    m_imageUrls = article.imageUrls;
    m_imageDescriptions = article.imageDescriptions;
    m_articleTitle = article.title;
    m_currentPageId = article.pageid;
    emit imageUrlsChanged();
    emit imageDescriptionsChanged();
    emit articleTitleChanged();
    emit currentPageIdChanged();
}
