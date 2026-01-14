// WikipediaClient.h
#ifndef WIKIPEDIA_CLIENT_H
#define WIKIPEDIA_CLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QVector>
#include <QMetaType>

namespace Wikipedia
{

    struct SearchResult
    {
        QString title;
        QString snippet;
        int pageid;
    };

    struct Page
    {
        QString title;
        QString extract;
        int pageid;
        QStringList imageUrls;
    };

    struct FeaturedArticle
    {
        QString title;
        QString extract;
        int pageid;
        QStringList imageUrls;
        QString date;
    };

} // namespace Wikipedia

Q_DECLARE_METATYPE(Wikipedia::SearchResult)
Q_DECLARE_METATYPE(Wikipedia::Page)
Q_DECLARE_METATYPE(Wikipedia::FeaturedArticle)

namespace Wikipedia {

    class WikipediaClient : public QObject
    {
        Q_OBJECT
    public:
        explicit WikipediaClient(QObject *parent = nullptr);
        ~WikipediaClient();

        void search(const QString &query, int limit = 10);
        void getPage(const QString &title);
        void getPageById(int pageid);
        void getPageWithImages(int pageid);
        void getFeaturedArticleOfTheDay();

    signals:
        void searchCompleted(const QVector<SearchResult> &results);
        void pageReceived(const Page &page);
        void pageWithImagesReceived(const Page &page);
        void featuredArticleReceived(const FeaturedArticle &article);
        void errorOccurred(const QString &error);

    private slots:
        void onSearchReply(QNetworkReply *reply);
        void onPageReply(QNetworkReply *reply, const QString &title);
        void onPageWithImagesReply(QNetworkReply *reply, int pageid);
        void onFeaturedArticleReply(QNetworkReply *reply);

    private:
        QNetworkAccessManager *networkManager;
        QString baseUrl;
        void fetchImageUrlsFromTitles(const QStringList &imageTitles, QStringList &imageUrls);
    };

} // namespace Wikipedia

#endif // WIKIPEDIA_CLIENT_H
