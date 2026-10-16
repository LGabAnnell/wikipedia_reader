// wikipedia_page_client.h
#ifndef WIKIPEDIA_PAGE_CLIENT_H
#define WIKIPEDIA_PAGE_CLIENT_H

#include "wikipedia_models.h"
#include "wikipedia_request_context.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QVector>
#include <functional>

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
    void setLanguage(const QString &langCode);
    void cancelPendingRequests();

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
    void onPageReply(QNetworkReply *reply, const QString &title, const WikipediaRequestContext &context);
    void onPageWithImagesReply(QNetworkReply *reply, int pageid, const WikipediaRequestContext &context);
    void onSectionsReply(QNetworkReply *reply, const WikipediaRequestContext &context);

  private:
    QNetworkAccessManager *networkManager;
    QString baseUrl;
    WikipediaRequestTracker m_requests;
    QHash<QString, QString> mathImageDataUrls;
    void fetchImageUrlsFromTitles(const QStringList &imageTitles, page pageData,
                                  const WikipediaRequestContext &context,
                                  std::function<void(page)> completion);
    void fetchPageContentWithImages(int pageid, const page &pageData, const WikipediaRequestContext &context);
    void fetchMathImageDataUrls(const QString &htmlContent,
                                const WikipediaRequestContext &context,
                                std::function<void(const QString &)> completion);
};

#endif // WIKIPEDIA_PAGE_CLIENT_H
