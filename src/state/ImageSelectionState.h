#ifndef IMAGESELECTIONSTATE_H
#define IMAGESELECTIONSTATE_H

#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QStringList>
#include "SettingsState.h"

class WikipediaPageClient;

/** @brief Shared selection for the fullscreen image view. */
class ImageSelectionState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Singleton")
    Q_PROPERTY(QString currentImageUrl READ currentImageUrl NOTIFY currentImageUrlChanged)
    Q_PROPERTY(QString currentImageDescription READ currentImageDescription NOTIFY currentImageDescriptionChanged)
    Q_PROPERTY(int currentGalleryIndex READ currentGalleryIndex NOTIFY gallerySelectionChanged)
    Q_PROPERTY(int galleryImageCount READ galleryImageCount NOTIFY gallerySelectionChanged)

  public:
    explicit ImageSelectionState(SettingsState &settings, QObject *parent = nullptr);
    QString currentImageUrl() const { return m_url; }
    QString currentImageDescription() const { return m_description; }
    static QPointer<ImageSelectionState> instance() { return m_instance; }
    int currentGalleryIndex() const { return m_galleryIndex; }
    int galleryImageCount() const { return m_galleryUrls.size(); }
    /** @brief Set URL and caption together before notifying observers. */
    Q_INVOKABLE void selectImage(const QString &url, const QString &description);
    /** @brief Select an image from an ordered article gallery. */
    Q_INVOKABLE void selectGalleryImage(const QStringList &urls, const QStringList &descriptions,
                                        const QStringList &titles, int index);
    /** @brief Select the next image when a gallery image is active. */
    Q_INVOKABLE void selectNextImage();
    /** @brief Select the previous image when a gallery image is active. */
    Q_INVOKABLE void selectPreviousImage();

  signals:
    void currentImageUrlChanged();
    void currentImageDescriptionChanged();
    void gallerySelectionChanged();

  private:
    QString m_url;
    QString m_description;
    QStringList m_galleryUrls;
    QStringList m_galleryDescriptions;
    QStringList m_galleryTitles;
    QString m_pendingDetailsTitle;
    int m_galleryIndex = -1;
    WikipediaPageClient *m_pageClient = nullptr;
    void updateSelection(const QString &url, const QString &description);
    void fetchDetailsForCurrentImage();
    static QPointer<ImageSelectionState> m_instance;
};

#endif // IMAGESELECTIONSTATE_H
