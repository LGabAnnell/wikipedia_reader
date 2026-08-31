#include "ImageHomeModel.h"
#include "wikipedia_page_client.h"
#include "state/GlobalState.h"

ImageHomeModel::ImageHomeModel(QObject *parent) : QObject(parent), m_currentPageId(-1) {
    // Connect to WikipediaPageClient from GlobalState
    m_pageClient = GlobalState::instance()->pageClient();
    
    // Connect to GlobalState's currentPageChanged signal to update when page changes
    connect(GlobalState::instance(), &GlobalState::currentPageChanged,
            this, &ImageHomeModel::handlePageWithImagesReceived);
    
    // Also connect to WikipediaPageClient's signal for direct updates
    if (m_pageClient) {
        connect(m_pageClient, &WikipediaPageClient::pageWithImagesReceived,
                this, [this](const page &p) {
                    m_imageUrls = p.imageUrls;
                    m_imageDescriptions = p.imageDescriptions;
                    m_articleTitle = p.title;
                    m_currentPageId = p.pageid;
                    emit imageUrlsChanged();
                    emit imageDescriptionsChanged();
                    emit articleTitleChanged();
                    emit currentPageIdChanged();
                });
    }
}

QStringList ImageHomeModel::imageUrls() const {
    return m_imageUrls;
}

QStringList ImageHomeModel::imageDescriptions() const {
    return m_imageDescriptions;
}

QString ImageHomeModel::articleTitle() const {
    return m_articleTitle;
}

int ImageHomeModel::currentPageId() const {
    return m_currentPageId;
}

void ImageHomeModel::loadImagesForCurrentPage() {
    // Get the current page ID from GlobalState
    int pageId = GlobalState::instance()->currentPageId();
    if (pageId > 0) {
        loadImagesForPage(pageId);
    }
}

void ImageHomeModel::loadImagesForPage(int pageId) {
    if (pageId <= 0 || !m_pageClient) return;
    
    m_currentPageId = pageId;
    emit currentPageIdChanged();
    
    // Fetch page with images
    m_pageClient->getPageWithImages(pageId);
}

void ImageHomeModel::handlePageWithImagesReceived() {
    // Get current page data from GlobalState
    page currentPage;
    currentPage.title = GlobalState::instance()->currentPageTitle();
    currentPage.extract = GlobalState::instance()->currentPageExtract();
    currentPage.pageid = GlobalState::instance()->currentPageId();
    currentPage.imageUrls = GlobalState::instance()->currentPageImageUrls();
    
    m_imageUrls = currentPage.imageUrls;
    m_imageDescriptions = currentPage.imageDescriptions;
    m_articleTitle = currentPage.title;
    m_currentPageId = currentPage.pageid;
    
    emit imageUrlsChanged();
    emit imageDescriptionsChanged();
    emit articleTitleChanged();
    emit currentPageIdChanged();
}
