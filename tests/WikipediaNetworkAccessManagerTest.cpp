#include "wikipedia_network_access_manager.h"

#include <QNetworkReply>
#include <QQmlEngine>
#include <QTemporaryFile>
#include <QtTest/QtTest>

class WikipediaNetworkAccessManagerTest : public QObject {
    Q_OBJECT

  private slots:
    void requestsUseRuntimeNetworkPolicy_data() {
        QTest::addColumn<bool>("qml");
        QTest::newRow("api-client") << false;
        QTest::newRow("qml-images") << true;
    }

    void requestsUseRuntimeNetworkPolicy() {
        QFETCH(bool, qml);
        QTemporaryFile file;
        QVERIFY(file.open());
        QCOMPARE(file.write("fixture"), 7);
        QVERIFY(file.flush());

        WikipediaNetwork::NetworkAccessManagerFactory factory;
        QQmlEngine engine;
        engine.setNetworkAccessManagerFactory(&factory);
        QObject owner;
        auto *manager = qml ? engine.networkAccessManager()
                            : WikipediaNetwork::createNetworkAccessManager(&owner);
        auto *reply = manager->get(QNetworkRequest(QUrl::fromLocalFile(file.fileName())));
        QSignalSpy finished(reply, &QNetworkReply::finished);
        QTRY_COMPARE(finished.count(), 1);
        QCOMPARE(reply->error(), QNetworkReply::NoError);
        QCOMPARE(reply->readAll(), QByteArray("fixture"));
        const QVariant http2 = reply->request().attribute(QNetworkRequest::Http2AllowedAttribute);
        if (QLibraryInfo::version() == QVersionNumber(6, 11, 2)) {
            QVERIFY(http2.isValid());
            QCOMPARE(http2.toBool(), false);
        } else {
            QVERIFY(!http2.isValid());
        }
        reply->deleteLater();
    }
};

QTEST_GUILESS_MAIN(WikipediaNetworkAccessManagerTest)
#include "WikipediaNetworkAccessManagerTest.moc"
