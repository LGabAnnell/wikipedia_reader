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
#include <QDate>

// ... existing includes ...

// New data structures for News, On This Day, and Did You Know
class news_item {
    Q_GADGET
        QML_ELEMENT
        Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString imageUrl MEMBER imageUrl)
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QString url MEMBER url)
public:
    QString title;
    QString imageUrl;
    QString description;
    QString url;
};

class on_this_day_event {
    Q_GADGET
        QML_ELEMENT
    Q_PROPERTY(int year MEMBER year)
    Q_PROPERTY(QString event MEMBER event)
    Q_PROPERTY(QString url MEMBER url)
public:
    int year;
    QString event;
    QString url;
};

class did_you_know_item {
    Q_GADGET
        QML_ELEMENT
    Q_PROPERTY(QString text MEMBER text)
    Q_PROPERTY(QString url MEMBER url)
public:
    QString text;
    QString url;
};

class history_item {
    Q_GADGET
        QML_ELEMENT
        Q_PROPERTY(QString title MEMBER title)
        Q_PROPERTY(int pageId MEMBER pageId)
        Q_PROPERTY(QDateTime timestamp MEMBER timestamp)
public:
    QString title;
    int pageId;
    QDateTime timestamp;
};

class search_result
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

class page
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

class featured_article
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

    // Existing methods
    void search(const QString &query, int limit = 10);
    void getPage(const QString &title);
    void getPageById(int pageid);
    void getPageWithImages(int pageid);
    void getFeaturedArticleOfTheDay();

    // New methods for HomeModel
    void getNewsItems();
    void getOnThisDayEvents();
    void getDidYouKnowItems();

    // Fetch news items from the featured feed
    void getNewsItems(const QDate &date);

    // Fetch on-this-day events
    void getOnThisDayEvents(int month, int day);

signals:
    // Existing signals
    void searchCompleted(const QVector<search_result> &results);
    void pageReceived(const page &page);
    void pageWithImagesReceived(const page &page);
    void featuredArticleReceived(const QString &title, const QString &extract, const int &pageid);
    void errorOccurred(const QString &error);

    // New signals for HomeModel
    void newsItemsReceived(const QVector<news_item> &items);
    void onThisDayEventsReceived(const QVector<on_this_day_event> &events);
    void didYouKnowItemsReceived(const QVector<did_you_know_item> &items);

private slots:
    // Existing slots
    void onSearchReply(QNetworkReply *reply);
    void onPageReply(QNetworkReply *reply, const QString &title);
    void onPageWithImagesReply(QNetworkReply *reply, int pageid);
    void onFeaturedArticleReply(QNetworkReply *reply);

    // New slots for HomeModel
    void onNewsItemsReply(QNetworkReply *reply);
    void onOnThisDayEventsReply(QNetworkReply *reply);
    void onRandomArticleTitleReply(QNetworkReply *reply);
    void onArticleContentReply(QNetworkReply *reply, const QString &title);

private:
    QNetworkAccessManager *networkManager;
    QString baseUrl;
    void fetchImageUrlsFromTitles(const QStringList &imageTitles, QStringList &imageUrls);

    // Helper method to fetch a random article title
    void fetchRandomArticleTitle();

    // Helper method to fetch article content
    void fetchArticleContent(const QString &title);
};

#endif // WIKIPEDIA_CLIENT_H

