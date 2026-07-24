#ifndef IMAGEHOMEMODEL_H
#define IMAGEHOMEMODEL_H

#include <QQmlEngine>
#include <QStringList>
#include <QObject>
#include "wikipedia_models.h"

class WikipediaPageClient;

class ImageHomeModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QStringList imageUrls READ imageUrls NOTIFY imageUrlsChanged)
    Q_PROPERTY(QString articleTitle READ articleTitle NOTIFY articleTitleChanged)
    Q_PROPERTY(int currentPageId READ currentPageId NOTIFY currentPageIdChanged)

public:
    explicit ImageHomeModel(QObject *parent = nullptr);

    QStringList imageUrls() const;
    QString articleTitle() const;
    int currentPageId() const;

    Q_INVOKABLE void loadImagesForCurrentPage();
    Q_INVOKABLE void loadImagesForPage(int pageId);

signals:
    void imageUrlsChanged();
    void articleTitleChanged();
    void currentPageIdChanged();

private slots:
    void handlePageWithImagesReceived();

private:
    WikipediaPageClient* m_pageClient;
    QStringList m_imageUrls;
    QString m_articleTitle;
    int m_currentPageId;
};

#endif // IMAGEHOMEMODEL_H

