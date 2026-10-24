// wikipedia_page_client.h
#ifndef WIKIPEDIA_PAGE_CLIENT_H
#define WIKIPEDIA_PAGE_CLIENT_H

#include "wikipedia_models.h"
#include "wikipedia_request_context.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHash>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QVector>
#include <QVariantMap>
#include <functional>

struct WikipediaPageClientLimits {
    qsizetype formulaCacheEntries = 256;
    qsizetype formulaCacheBytes = 16 * 1024 * 1024;
    int formulaEnrichmentTimeoutMs = 3'000;
};

class WikipediaPageClient : public QObject {
    Q_OBJECT
  public:
    explicit WikipediaPageClient(QObject *parent = nullptr);
    WikipediaPageClient(QObject *parent, const WikipediaPageClientLimits &limits);
    ~WikipediaPageClient();

    void getPage(const QString &title);
    void getPageById(int pageid);
    void getPageById(int pageid, const WikipediaRequestContext &context);
    void getPageWithImages(int pageid, const QVariantMap &continuation = {});
    /** @brief Fetch a larger preview and caption for a selected file. */
    void getImageDetails(const QString &title);
    void resolveTitleToPageId(const QString &title);
    void getSections(const QString &title);
    void setLanguage(const QString &langCode);
    void cancelPendingRequests();
    qsizetype mathImageCacheEntryCount() const { return mathImageDataUrls.size(); }
    qsizetype mathImageCachePayloadBytes() const { return mathImageCacheBytes; }

    static page parsePage(const QByteArray &responseData, const QString &title);
    static page parsePageWithImages(const QByteArray &responseData, int pageid);
    static QVector<section> parseSections(const QByteArray &responseData);

  signals:
    void pageReceived(const page &page);
    void pageWithImagesReceived(const page &page);
    void imageDetailsReceived(const QString &title, const QString &url, const QString &description);
    void pageIdResolved(int pageid);
    void pageIdResolvedWithContext(int pageid, const WikipediaRequestContext &context);
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
    WikipediaPageClientLimits m_limits;
    QList<QString> mathImageRecency;
    qsizetype mathImageCacheBytes = 0;
    void fetchImageUrlsFromTitles(const QStringList &imageTitles, page pageData,
                                  const WikipediaRequestContext &context,
                                  std::function<void(page)> completion, bool details = false);
    void fetchPageContentWithImages(int pageid, const page &pageData, const WikipediaRequestContext &context);
    void fetchMathImageDataUrls(const QString &htmlContent,
                                const WikipediaRequestContext &context,
                                std::function<void(const QString &)> completion);
};

#endif // WIKIPEDIA_PAGE_CLIENT_H
