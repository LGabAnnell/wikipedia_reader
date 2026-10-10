#ifndef IMAGEHOMEMODEL_H
#define IMAGEHOMEMODEL_H

#include <QQmlEngine>
#include <QStringList>
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
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
    Q_PROPERTY(QStringList imageUrls READ imageUrls NOTIFY imageUrlsChanged)
    Q_PROPERTY(QStringList imageDescriptions READ imageDescriptions NOTIFY imageDescriptionsChanged)
    Q_PROPERTY(QString articleTitle READ articleTitle NOTIFY articleTitleChanged)
    Q_PROPERTY(int currentPageId READ currentPageId NOTIFY currentPageIdChanged)

public:
    explicit ImageHomeModel(QObject *parent = nullptr);

    bool isLoading() const { return m_isLoading; }
    QString errorMessage() const { return m_errorMessage; }
    QStringList imageUrls() const;
    QStringList imageDescriptions() const;
    QString articleTitle() const;
    int currentPageId() const;

    Q_INVOKABLE void loadImagesForCurrentPage();
    Q_INVOKABLE void loadImagesForPage(int pageId);

signals:
    void loadingChanged();
    void errorChanged();
    void imageUrlsChanged();
    void imageDescriptionsChanged();
    void articleTitleChanged();
    void currentPageIdChanged();

private slots:
    void handlePageWithImagesReceived();

private:
    QPointer<ArticleState> m_article;
    QPointer<SettingsState> m_settings;
    WikipediaPageClient *m_pageClient;
    bool m_isLoading = false;
    QString m_errorMessage;
    QStringList m_imageUrls;
    QStringList m_imageDescriptions;
    QString m_articleTitle;
    int m_currentPageId;
};

#endif // IMAGEHOMEMODEL_H

