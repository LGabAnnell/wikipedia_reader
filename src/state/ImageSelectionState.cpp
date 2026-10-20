#include "ImageSelectionState.h"
#include "wikipedia_page_client.h"

QPointer<ImageSelectionState> ImageSelectionState::m_instance;

ImageSelectionState::ImageSelectionState(SettingsState &settings, QObject *parent) : QObject(parent),
    m_pageClient(new WikipediaPageClient(this)) {
    Q_ASSERT(!m_instance);
    m_instance = this;
    m_pageClient->setLanguage(settings.language());
    connect(&settings, &SettingsState::languageTransitionStarted, this, [this]() {
        selectImage({}, {});
    });
    connect(&settings, &SettingsState::languageChanged, this, [this, &settings]() {
        m_pageClient->setLanguage(settings.language());
    });
    connect(m_pageClient, &WikipediaPageClient::imageDetailsReceived, this,
            [this](const QString &title, const QString &url, const QString &description) {
        if (m_galleryIndex < 0 || title != m_galleryTitles.value(m_galleryIndex))
            return;
        m_pendingDetailsTitle.clear();
        updateSelection(url.isEmpty() ? m_galleryUrls.value(m_galleryIndex) : url,
                        description.isEmpty() ? m_galleryDescriptions.value(m_galleryIndex) : description);
    });
    connect(m_pageClient, &WikipediaPageClient::errorOccurred, this, [this](const QString &) {
        if (m_pendingDetailsTitle.isEmpty() || m_pendingDetailsTitle != m_galleryTitles.value(m_galleryIndex))
            return;
        m_pendingDetailsTitle.clear();
        updateSelection(m_galleryUrls.value(m_galleryIndex), m_galleryDescriptions.value(m_galleryIndex));
    });
}

void ImageSelectionState::selectImage(const QString &url, const QString &description) {
    m_pageClient->cancelPendingRequests();
    m_pendingDetailsTitle.clear();
    m_galleryUrls.clear();
    m_galleryDescriptions.clear();
    m_galleryTitles.clear();
    const bool hadGallery = m_galleryIndex != -1;
    m_galleryIndex = -1;
    if (hadGallery)
        emit gallerySelectionChanged();
    updateSelection(url, description);
}

void ImageSelectionState::updateSelection(const QString &url, const QString &description) {
    const bool urlChanged = m_url != url;
    const bool descriptionChanged = m_description != description;
    m_url = url;
    m_description = description;
    if (urlChanged)
        emit currentImageUrlChanged();
    if (descriptionChanged)
        emit currentImageDescriptionChanged();
}

void ImageSelectionState::selectGalleryImage(const QStringList &urls, const QStringList &descriptions,
                                             const QStringList &titles, int index) {
    if (index < 0 || index >= urls.size())
        return;
    const bool changedGallery = m_galleryUrls != urls || m_galleryTitles != titles
        || m_galleryDescriptions != descriptions;
    m_galleryUrls = urls;
    m_galleryDescriptions = descriptions;
    m_galleryTitles = titles;
    const bool indexChanged = m_galleryIndex != index;
    m_galleryIndex = index;
    if (!changedGallery && !indexChanged)
        return;
    m_pageClient->cancelPendingRequests();
    m_pendingDetailsTitle.clear();
    if (changedGallery || indexChanged)
        emit gallerySelectionChanged();
    const QString title = titles.value(index);
    if (title.isEmpty())
        updateSelection(urls.at(index), descriptions.value(index));
    else
        m_pendingDetailsTitle = title;
    fetchDetailsForCurrentImage();
}

void ImageSelectionState::selectNextImage() {
    if (m_galleryIndex >= 0 && m_galleryIndex + 1 < m_galleryUrls.size())
        selectGalleryImage(m_galleryUrls, m_galleryDescriptions, m_galleryTitles, m_galleryIndex + 1);
}

void ImageSelectionState::selectPreviousImage() {
    if (m_galleryIndex > 0)
        selectGalleryImage(m_galleryUrls, m_galleryDescriptions, m_galleryTitles, m_galleryIndex - 1);
}

void ImageSelectionState::fetchDetailsForCurrentImage() {
    if (m_galleryIndex < 0)
        return;
    const QString title = m_galleryTitles.value(m_galleryIndex);
    if (!title.isEmpty())
        m_pageClient->getImageDetails(title);
}
