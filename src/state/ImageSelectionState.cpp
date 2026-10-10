#include "ImageSelectionState.h"

QPointer<ImageSelectionState> ImageSelectionState::m_instance;

ImageSelectionState::ImageSelectionState(QObject *parent) : QObject(parent) {
    Q_ASSERT(!m_instance);
    m_instance = this;
}

void ImageSelectionState::selectImage(const QString &url, const QString &description) {
    const bool urlChanged = m_url != url;
    const bool descriptionChanged = m_description != description;
    m_url = url;
    m_description = description;
    if (urlChanged)
        emit currentImageUrlChanged();
    if (descriptionChanged)
        emit currentImageDescriptionChanged();
}
