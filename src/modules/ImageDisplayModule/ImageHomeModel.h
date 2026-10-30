#ifndef IMAGEHOMEMODEL_H
#define IMAGEHOMEMODEL_H

#include <QQmlEngine>
#include <QStringList>
#include <QSet>
#include <QObject>
#include "wikipedia_models.h"
#include "ArticleState.h"
#include "SettingsState.h"

class WikipediaPageClient;

class ImageHomeModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(bool isLoadingMore READ isLoadingMore NOTIFY loadingMoreChanged)
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY hasMoreChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
    Q_PROPERTY(QStringList imageUrls READ imageUrls NOTIFY imageUrlsChanged)
    Q_PROPERTY(QStringList imageDescriptions READ imageDescriptions NOTIFY imageDescriptionsChanged)
    Q_PROPERTY(QString articleTitle READ articleTitle NOTIFY articleTitleChanged)
    Q_PROPERTY(int currentPageId READ currentPageId NOTIFY currentPageIdChanged)

public:
    explicit ImageHomeModel(QObject *parent = nullptr);

    [[nodiscard]] bool isLoading() const { return m_isLoading; }
    [[nodiscard]] bool isLoadingMore() const { return m_isLoadingMore; }
    [[nodiscard]] bool hasMore() const { return !m_continuation.isEmpty(); }
    [[nodiscard]] QString errorMessage() const { return m_errorMessage; }
    [[nodiscard]] QStringList imageUrls() const;
    [[nodiscard]] QStringList imageDescriptions() const;
    [[nodiscard]] QString articleTitle() const;
    [[nodiscard]] int currentPageId() const;

    Q_INVOKABLE void loadImagesForCurrentPage();
    Q_INVOKABLE void loadImagesForPage(int pageId);
    Q_INVOKABLE void loadMore();
    /** @brief Select a thumbnail and fetch its fullscreen preview and caption. */
    Q_INVOKABLE void selectImage(int index) const;

signals:
    void loadingChanged();
    void loadingMoreChanged();
    void hasMoreChanged();
    void errorChanged();
    void imageUrlsChanged();
    void imageDescriptionsChanged();
    void articleTitleChanged();
    void currentPageIdChanged();

private:
    void receiveGalleryBatch(const page &article, bool append);
    QPointer<ArticleState> m_article;
    QPointer<SettingsState> m_settings;
    WikipediaPageClient *m_pageClient;
    bool m_isLoading = false;
    bool m_isLoadingMore = false;
    QString m_errorMessage;
    QStringList m_imageUrls;
    QStringList m_imageTitles;
    QStringList m_imageDescriptions;
    QString m_articleTitle;
    int m_currentPageId;
    QVariantMap m_continuation;
    QVariantMap m_requestedContinuation;
    QSet<QString> m_imageTitlesSeen;
    QSet<QString> m_seenContinuations;
};

#endif // IMAGEHOMEMODEL_H

