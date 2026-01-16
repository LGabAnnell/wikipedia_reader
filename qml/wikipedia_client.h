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
#include <QQmlEngine>


class SearchResult
{
    Q_GADGET
    QML_ELEMENT
        Q_PROPERTY(QString title MEMBER title)
        Q_PROPERTY(QString snippet MEMBER snippet)
        Q_PROPERTY(int pageid MEMBER pageid)
public:
    QString title;
    QString snippet;
    int pageid;
};

class Page
{
    Q_GADGET
    QML_ELEMENT
        Q_PROPERTY(QString title MEMBER title)
        Q_PROPERTY(QString extract MEMBER extract)
        Q_PROPERTY(int pageid MEMBER pageid)
public:
    QString title;
    QString extract;
    int pageid;
    QStringList imageUrls;
};

class FeaturedArticle
{
    Q_GADGET
    QML_ELEMENT
        Q_PROPERTY(QString title MEMBER title)
        Q_PROPERTY(QString extract MEMBER extract)
        Q_PROPERTY(int pageid MEMBER pageid)
        Q_PROPERTY(QStringList imageUrls MEMBER imageUrls)
public:
    QString title;
    QString extract;
    int pageid;
    QStringList imageUrls;
    QString date;
};

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

#endif // WIKIPEDIA_CLIENT_H
