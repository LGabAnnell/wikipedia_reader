#include "wikipedia_featured_client.h"
#include "wikipedia_home_client.h"
#include "wikipedia_network_access_manager.h"

#include <QCoreApplication>
#include <QtTest/QtTest>

// Observe the real transport, without replacing any server response. Keep this
// endpoint contract independent of the offline fixture registrations.
class ObservedNetworkFactory : public QQmlNetworkAccessManagerFactory {
  public:
    QList<QUrl> completedRequests;

    QNetworkAccessManager *create(QObject *parent) override {
        auto *manager = new WikipediaNetwork::NetworkAccessManager(parent);
        QObject::connect(manager, &QNetworkAccessManager::finished, manager,
                         [this](QNetworkReply *reply) { completedRequests.append(reply->request().url()); });
        return manager;
    }
};

class WikipediaFeedLiveTest : public QObject {
    Q_OBJECT

    ObservedNetworkFactory m_network;

    QUrl expectedFeed(const QString &language) const {
        return QUrl(QString("https://api.wikimedia.org/feed/v1/wikipedia/%1/featured/%2")
                        .arg(language, QDate::currentDate().toString("yyyy/MM/dd")));
    }

  private slots:
    void initTestCase() { WikipediaNetwork::installNetworkAccessManagerFactory(&m_network); }
    void cleanupTestCase() { WikipediaNetwork::installNetworkAccessManagerFactory(nullptr); }
    void init() { m_network.completedRequests.clear(); }

    void englishFeaturedArticle() {
        WikipediaFeaturedClient client;
        QSignalSpy received(&client, &WikipediaFeaturedClient::featuredArticleReceived);
        QSignalSpy unavailable(&client, &WikipediaFeaturedClient::featuredArticleUnavailable);
        QSignalSpy errors(&client, &WikipediaFeaturedClient::errorOccurred);
        client.getFeaturedArticleOfTheDay();

        QTRY_VERIFY_WITH_TIMEOUT(received.count() + unavailable.count() + errors.count() > 0, 45000);
        QVERIFY2(m_network.completedRequests.contains(expectedFeed("en")),
                 "Featured client did not use the aggregated Wikimedia feed");
        QVERIFY2(errors.isEmpty(), errors.isEmpty() ? "" : qPrintable(errors.first().first().toString()));
        QVERIFY2(unavailable.isEmpty(), "English feed did not supply today's featured article");
        QCOMPARE(received.count(), 1);
        QVERIFY(!received.first().at(0).toString().trimmed().isEmpty());
        QVERIFY(!received.first().at(1).toString().trimmed().isEmpty());
        QVERIFY(received.first().at(2).toInt() > 0);
    }

    void homeArticles_data() {
        QTest::addColumn<QString>("language");
        QTest::newRow("English") << QString("en");
        QTest::newRow("French") << QString("fr");
    }

    void homeArticles() {
        QFETCH(QString, language);
        WikipediaHomeClient client;
        client.setLanguage(language);
        QSignalSpy received(&client, &WikipediaHomeClient::newsItemsReceived);
        QSignalSpy errors(&client, &WikipediaHomeClient::errorOccurred);
        client.getNewsItems();

        QTRY_VERIFY_WITH_TIMEOUT(received.count() + errors.count() > 0, 45000);
        QVERIFY2(m_network.completedRequests.contains(expectedFeed(language)),
                 "Home client did not use the aggregated Wikimedia feed");
        QVERIFY2(errors.isEmpty(), errors.isEmpty() ? "" : qPrintable(errors.first().first().toString()));
        QCOMPARE(received.count(), 1);
        const auto articles = received.first().first().value<QVector<news_item>>();
        QVERIFY2(!articles.isEmpty(), "Feed completed without any home articles");
        for (const auto &article : articles) {
            QVERIFY(!article.title.trimmed().isEmpty());
            QVERIFY(article.pageid > 0);
            QVERIFY(!article.description.trimmed().isEmpty());
            const QUrl url(article.url);
            QCOMPARE(url.scheme(), QString("https"));
            QCOMPARE(url.host(), language + ".wikipedia.org");
        }
    }
};

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    if (!qEnvironmentVariableIntValue("RUN_NETWORK_TESTS"))
        return 77;
    WikipediaFeedLiveTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "WikipediaFeedLiveTest.moc"
