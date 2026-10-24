#include "ImageHomeModel.h"
#include "wikipedia_page_client.h"
#include "ImageSelectionState.h"
#include <QJsonDocument>

namespace {
QString continuationKey(const QVariantMap &continuation) {
    return QString::fromUtf8(QJsonDocument::fromVariant(continuation).toJson(QJsonDocument::Compact));
}

QString canonicalImageTitle(const QString &title) {
    const qsizetype colon = title.indexOf(QLatin1Char(':'));
    if (colon < 0)
        return title.isEmpty() ? title : title.at(0).toCaseFolded() + title.mid(1);
    const QString nameSpace = title.left(colon).toCaseFolded();
    const QString name = title.mid(colon + 1);
    return nameSpace + QLatin1Char(':')
        + (name.isEmpty() ? name : name.at(0).toCaseFolded() + name.mid(1));
}
}

ImageHomeModel::ImageHomeModel(QObject *parent)
    : QObject(parent), m_article(ArticleState::instance()), m_settings(SettingsState::instance()),
      m_pageClient(new WikipediaPageClient(this)), m_currentPageId(-1) {
    if (m_settings) {
        m_pageClient->setLanguage(m_settings->language());
        connect(m_settings, &SettingsState::languageTransitionStarted, this, [this]() {
            m_pageClient->cancelPendingRequests();
            m_imageUrls.clear();
            m_imageTitles.clear();
            m_imageDescriptions.clear();
            m_imageTitlesSeen.clear();
            m_continuation.clear();
            m_requestedContinuation.clear();
            m_seenContinuations.clear();
            m_articleTitle.clear();
            m_currentPageId = -1;
            m_isLoading = false;
            m_isLoadingMore = false;
            m_errorMessage.clear();
            emit imageUrlsChanged();
            emit imageDescriptionsChanged();
            emit articleTitleChanged();
            emit currentPageIdChanged();
            emit loadingChanged();
            emit loadingMoreChanged();
            emit hasMoreChanged();
            emit errorChanged();
        });
        connect(m_settings, &SettingsState::languageChanged, this, [this]() {
            m_pageClient->setLanguage(m_settings->language());
        });
    }
    connect(m_pageClient, &WikipediaPageClient::pageWithImagesReceived, this, [this](const page &article) {
        receiveGalleryBatch(article, m_isLoadingMore);
    });
    connect(m_pageClient, &WikipediaPageClient::errorOccurred, this, [this](const QString &error) {
        m_errorMessage = error;
        m_isLoading = false;
        m_isLoadingMore = false;
        emit errorChanged();
        emit loadingChanged();
        emit loadingMoreChanged();
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
    m_pageClient->cancelPendingRequests();
    if (pageId <= 0) {
        const bool previousHasMore = hasMore();
        m_currentPageId = -1;
        m_continuation.clear();
        m_requestedContinuation.clear();
        m_seenContinuations.clear();
        m_isLoading = false;
        m_isLoadingMore = false;
        m_errorMessage = tr("Unable to load gallery for this article.");
        if (previousHasMore)
            emit hasMoreChanged();
        emit currentPageIdChanged();
        emit loadingChanged();
        emit loadingMoreChanged();
        emit errorChanged();
        return;
    }
    const bool previousHasMore = hasMore();
    m_currentPageId = pageId;
    m_imageUrls.clear();
    m_imageTitles.clear();
    m_imageDescriptions.clear();
    m_imageTitlesSeen.clear();
    m_continuation.clear();
    m_requestedContinuation.clear();
    m_seenContinuations.clear();
    m_isLoading = true;
    m_isLoadingMore = false;
    m_errorMessage.clear();
    emit currentPageIdChanged();
    emit imageUrlsChanged();
    emit imageDescriptionsChanged();
    emit loadingChanged();
    emit loadingMoreChanged();
    if (previousHasMore)
        emit hasMoreChanged();
    emit errorChanged();
    m_pageClient->getPageWithImages(pageId);
}

void ImageHomeModel::receiveGalleryBatch(const page &article, bool append) {
    const QString continuation = continuationKey(article.imageContinuation);
    if (append && !article.imageContinuation.isEmpty()
        && (article.imageContinuation == m_requestedContinuation || m_seenContinuations.contains(continuation))) {
        const bool previousHasMore = hasMore();
        m_continuation.clear();
        m_requestedContinuation.clear();
        m_errorMessage = tr("More images could not be loaded because Wikipedia repeated a gallery page.");
        m_isLoadingMore = false;
        if (previousHasMore != hasMore())
            emit hasMoreChanged();
        emit errorChanged();
        emit loadingMoreChanged();
        return;
    }
    const bool previousHasMore = hasMore();
    if (!append) {
        m_imageUrls.clear();
        m_imageTitles.clear();
        m_imageDescriptions.clear();
        m_imageTitlesSeen.clear();
    }
    for (qsizetype i = 0; i < article.imageUrls.size(); ++i) {
        const QString &url = article.imageUrls.at(i);
        const QString title = article.imageTitles.value(i);
        const QString identity = title.isEmpty() ? url : canonicalImageTitle(title);
        if (m_imageTitlesSeen.contains(identity))
            continue;
        m_imageTitlesSeen.insert(identity);
        m_imageTitles.append(title);
        m_imageDescriptions.append(article.imageDescriptions.value(i));
        m_imageUrls.append(url);
    }
    m_continuation = article.imageContinuation;
    if (!m_continuation.isEmpty())
        m_seenContinuations.insert(continuationKey(m_continuation));
    m_articleTitle = article.title;
    m_currentPageId = article.pageid;
    m_isLoading = false;
    m_isLoadingMore = false;
    m_errorMessage.clear();
    if (const auto selection = ImageSelectionState::instance())
        selection->refreshGalleryImages(m_currentPageId, m_imageUrls, m_imageDescriptions, m_imageTitles);
    emit imageUrlsChanged();
    emit imageDescriptionsChanged();
    emit articleTitleChanged();
    emit currentPageIdChanged();
    emit loadingChanged();
    emit loadingMoreChanged();
    emit errorChanged();
    if (previousHasMore != hasMore())
        emit hasMoreChanged();
}

void ImageHomeModel::loadMore() {
    if (m_isLoading || m_isLoadingMore || !hasMore() || m_currentPageId <= 0)
        return;
    m_isLoadingMore = true;
    m_requestedContinuation = m_continuation;
    m_errorMessage.clear();
    emit loadingMoreChanged();
    emit errorChanged();
    m_pageClient->getPageWithImages(m_currentPageId, m_continuation);
}

void ImageHomeModel::selectImage(int index) {
    if (index < 0 || index >= m_imageUrls.size() || !ImageSelectionState::instance())
        return;
    ImageSelectionState::instance()->selectGalleryImageForPage(m_imageUrls, m_imageDescriptions,
                                                               m_imageTitles, index, m_currentPageId);
}
