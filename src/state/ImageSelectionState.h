#ifndef IMAGESELECTIONSTATE_H
#define IMAGESELECTIONSTATE_H

#include <QObject>
#include <QPointer>
#include <QQmlEngine>

/** @brief Shared selection for the fullscreen image view. */
class ImageSelectionState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Singleton")
    Q_PROPERTY(QString currentImageUrl READ currentImageUrl NOTIFY currentImageUrlChanged)
    Q_PROPERTY(QString currentImageDescription READ currentImageDescription NOTIFY currentImageDescriptionChanged)

  public:
    explicit ImageSelectionState(QObject *parent = nullptr);
    QString currentImageUrl() const { return m_url; }
    QString currentImageDescription() const { return m_description; }
    static QPointer<ImageSelectionState> instance() { return m_instance; }
    /** @brief Set URL and caption together before notifying observers. */
    Q_INVOKABLE void selectImage(const QString &url, const QString &description);

  signals:
    void currentImageUrlChanged();
    void currentImageDescriptionChanged();

  private:
    QString m_url;
    QString m_description;
    static QPointer<ImageSelectionState> m_instance;
};

#endif // IMAGESELECTIONSTATE_H
