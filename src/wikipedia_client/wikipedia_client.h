// WikipediaClient.h

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QVector>

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

    signals:
        void searchCompleted(const QVector<SearchResult> &results);
        void pageReceived(const Page &page);
        void errorOccurred(const QString &error);

    private slots:
        void onSearchReply(QNetworkReply *reply);
        void onPageReply(QNetworkReply *reply, const QString &title);

    private:
        QNetworkAccessManager *networkManager;
        QString baseUrl;
    };

} // namespace Wikipedia