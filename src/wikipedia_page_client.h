// wikipedia_page_client.h
#ifndef WIKIPEDIA_PAGE_CLIENT_H
#define WIKIPEDIA_PAGE_CLIENT_H

#include "wikipedia_models.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QVector>

class WikipediaPageClient : public QObject {
    Q_OBJECT
  public:
    explicit WikipediaPageClient(QObject *parent = nullptr);
    ~WikipediaPageClient();

    void getPage(const QString &title);
    void getPageById(int pageid);
    void getPageWithImages(int pageid);
    void resolveTitleToPageId(const QString &title);
    void getSections(const QString &title);

    static page parsePage(const QByteArray &responseData, const QString &title);
    static page parsePageWithImages(const QByteArray &responseData, int pageid);
    static QVector<section> parseSections(const QByteArray &responseData);

  signals:
    void pageReceived(const page &page);
    void pageWithImagesReceived(const page &page);
    void pageIdResolved(int pageid);
    void sectionsReceived(const QVector<section> &sections);
    void errorOccurred(const QString &error);

  private slots:
    void onPageReply(QNetworkReply *reply, const QString &title);
    void onPageWithImagesReply(QNetworkReply *reply, int pageid);
    void onSectionsReply(QNetworkReply *reply);

  private:
    QNetworkAccessManager *networkManager;
    QString baseUrl;
    void fetchImageUrlsFromTitles(const QStringList &imageTitles, QStringList &imageUrls,
                                  QStringList &imageDescriptions);
    void fetchPageContentWithImages(int pageid, const page &pageData);
};

#endif // WIKIPEDIA_PAGE_CLIENT_H
