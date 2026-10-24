#include "wikipedia_network_access_manager.h"

#include <QNetworkReply>
#include <QNetworkProxy>
#include <QElapsedTimer>
#include <QHash>
#include <QHostAddress>
#include <QPointer>
#include <QQmlEngine>
#include <QScopeGuard>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QTemporaryFile>
#include <QtTest/QtTest>
#include <algorithm>

namespace {
constexpr auto DeadlineAttribute = static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 1);
constexpr auto MaximumResponseBytesAttribute = static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 2);

QNetworkRequest localRequest(const QUrl &url, int deadlineMs = 3000, int timeoutMs = 250) {
    QNetworkRequest request(url);
    request.setAttribute(DeadlineAttribute, wikipediaMonotonicNowMs() + deadlineMs);
    request.setTransferTimeout(timeoutMs);
    return request;
}

void useDirectTransport(QNetworkAccessManager &manager) {
    manager.setProxy(QNetworkProxy::NoProxy);
}

class LocalHttpServer final : public QObject {
    Q_OBJECT
  public:
    explicit LocalHttpServer(QObject *parent = nullptr) : QObject(parent) {
        connect(&m_server, &QTcpServer::newConnection, this, [this] {
            while (QTcpSocket *socket = m_server.nextPendingConnection()) {
                connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
                    if (m_paths.contains(socket))
                        return;
                    m_buffers[socket].append(socket->readAll());
                    const QByteArray request = m_buffers.value(socket);
                    if (!request.contains("\r\n\r\n"))
                        return;
                    const QByteArray firstLine = request.left(request.indexOf("\r\n"));
                    const QList<QByteArray> parts = firstLine.split(' ');
                    const QString path = parts.size() > 1 ? QString::fromUtf8(parts.at(1)) : QString();
                    m_paths.insert(socket, path);
                    m_requests.append(socket);
                    emit requestReceived(path);
                });
            }
        });
    }

    bool start() { return m_server.listen(QHostAddress::AnyIPv4, 0); }
    quint16 port() const { return m_server.serverPort(); }
    int requestCount() const { return m_requests.size(); }
    QString requestPath(int index) const {
        return index >= 0 && index < m_requests.size() ? m_paths.value(m_requests.at(index)) : QString();
    }
    int countPath(const QString &path) const {
        return int(std::count_if(m_requests.cbegin(), m_requests.cend(), [this, &path](const auto &socket) {
            return socket && m_paths.value(socket) == path;
        }));
    }
    QTcpSocket *socketForPath(const QString &path, int occurrence = 0) const {
        for (const auto &socket : m_requests) {
            if (socket && m_paths.value(socket) == path && occurrence-- == 0)
                return socket;
        }
        return nullptr;
    }
    QUrl url(const QString &path, const QString &host = QStringLiteral("127.0.0.1")) const {
        return QUrl(QStringLiteral("http://%1:%2%3").arg(host).arg(m_server.serverPort()).arg(path));
    }
    void respond(QTcpSocket *socket, int status = 200, const QByteArray &body = "ok",
                 const QList<QPair<QByteArray, QByteArray>> &headers = {}) {
        if (!socket || socket->state() == QAbstractSocket::UnconnectedState)
            return;
        QByteArray reason = status == 429 ? "Too Many Requests"
                            : status == 503 ? "Service Unavailable"
                            : status == 500 ? "Internal Server Error" : "OK";
        QByteArray response = "HTTP/1.1 " + QByteArray::number(status) + " " + reason + "\r\n";
        response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
        response += "Connection: close\r\n";
        for (const auto &header : headers)
            response += header.first + ": " + header.second + "\r\n";
        response += "\r\n" + body;
        socket->write(response);
        socket->flush();
        socket->disconnectFromHost();
    }
    void sendSlowBody(QTcpSocket *socket) {
        if (!socket)
            return;
        socket->write("HTTP/1.1 200 OK\r\nContent-Length: 50\r\nConnection: keep-alive\r\n\r\nx");
        socket->flush();
        auto *timer = new QTimer(socket);
        timer->setInterval(35);
        connect(timer, &QTimer::timeout, socket, [socket, timer] {
            if (socket->state() != QAbstractSocket::ConnectedState) {
                timer->stop();
                return;
            }
            socket->write("x");
            socket->flush();
        });
        timer->start();
    }

  signals:
    void requestReceived(const QString &path);

  private:
    QTcpServer m_server;
    QHash<QTcpSocket *, QByteArray> m_buffers;
    QHash<QTcpSocket *, QString> m_paths;
    QList<QPointer<QTcpSocket>> m_requests;
};
} // namespace

class WikipediaNetworkAccessManagerTest : public QObject {
    Q_OBJECT

  private slots:
    void requestsUseRuntimeNetworkPolicy_data() {
        QTest::addColumn<bool>("qml");
        QTest::addColumn<bool>("installed");
        QTest::newRow("api-client") << false << false;
        QTest::newRow("qml-images") << true << false;
        QTest::newRow("api-client-installed-factory") << false << true;
        QTest::newRow("qml-images-installed-factory") << true << true;
    }

    void requestsUseRuntimeNetworkPolicy() {
        QFETCH(bool, qml);
        QFETCH(bool, installed);
        QTemporaryFile file;
        QVERIFY(file.open());
        QCOMPARE(file.write("fixture"), 7);
        QVERIFY(file.flush());

        WikipediaNetwork::NetworkAccessManagerFactory factory;
        auto *application = QCoreApplication::instance();
        const QVariant previousFactory = application->property(WikipediaNetwork::FactoryProperty);
        const auto restoreFactory = qScopeGuard([application, previousFactory] {
            application->setProperty(WikipediaNetwork::FactoryProperty, previousFactory);
        });
        WikipediaNetwork::installNetworkAccessManagerFactory(installed ? &factory : nullptr);
        QQmlEngine engine;
        engine.setNetworkAccessManagerFactory(&factory);
        QObject owner;
        auto *manager = qml ? engine.networkAccessManager()
                            : WikipediaNetwork::createNetworkAccessManager(&owner);
        QVERIFY(dynamic_cast<WikipediaNetwork::NetworkAccessManager *>(manager));
        if (!qml) {
            QCOMPARE(manager->parent(), &owner);
        }
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

    void productionQueueLimitsConcurrencyAcrossManagersAndReleasesOnDestruction() {
        LocalHttpServer server;
        QVERIFY(server.start());
        WikipediaNetwork::NetworkAccessManager managers[6];
        for (auto &manager : managers)
            useDirectTransport(manager);
        QNetworkReply *first = managers[0].get(localRequest(server.url("/stall-1"), 5000, 3000));
        QNetworkReply *second = managers[1].get(localRequest(server.url("/stall-2"), 5000, 3000));
        QNetworkReply *third = managers[2].get(localRequest(server.url("/stall-3"), 5000, 3000));
        QSignalSpy firstFinished(first, &QNetworkReply::finished);
        QSignalSpy firstErrors(first, &QNetworkReply::errorOccurred);
        QTRY_COMPARE_WITH_TIMEOUT(server.requestCount(), 3, 1000);

        QNetworkReply *fourth = managers[3].get(localRequest(server.url("/released-slot"), 5000, 3000));
        QSignalSpy fourthFinished(fourth, &QNetworkReply::finished);
        QTest::qWait(150);
        QCOMPARE(server.requestCount(), 3);

        delete first; // Destruction of an active wrapper must release its slot.
        QCOMPARE(firstFinished.count(), 0);
        QCOMPARE(firstErrors.count(), 0);
        QTRY_COMPARE_WITH_TIMEOUT(server.requestCount(), 4, 1000);
        QCOMPARE(server.requestPath(3), QStringLiteral("/released-slot"));

        QNetworkReply *fifth = managers[4].get(localRequest(server.url("/canceled-before-dispatch"), 5000, 3000));
        QSignalSpy fifthFinished(fifth, &QNetworkReply::finished);
        QSignalSpy fifthErrors(fifth, &QNetworkReply::errorOccurred);
        fifth->abort();
        QTRY_COMPARE(fifthFinished.count(), 1);
        QCOMPARE(fifthErrors.count(), 1);
        QTest::qWait(150);
        QCOMPARE(server.countPath("/canceled-before-dispatch"), 0);

        QNetworkReply *sixth = managers[5].get(localRequest(server.url("/destroyed-before-dispatch"), 5000, 3000));
        QSignalSpy sixthFinished(sixth, &QNetworkReply::finished);
        delete sixth;
        QCOMPARE(sixthFinished.count(), 0);
        QTest::qWait(150);
        QCOMPARE(server.countPath("/destroyed-before-dispatch"), 0);

        server.respond(server.socketForPath("/released-slot"));
        QTRY_COMPARE(fourthFinished.count(), 1);
        QCOMPARE(fourth->error(), QNetworkReply::NoError);
        QCOMPARE(server.requestCount(), 4);
        second->abort();
        third->abort();
        delete fourth;
        delete fifth;
    }

    void inactivityTimeoutReleasesSlotAndAllowsRecovery() {
        LocalHttpServer server;
        QVERIFY(server.start());
        WikipediaNetwork::NetworkAccessManager manager;
        useDirectTransport(manager);
        QNetworkReply *stalled = manager.get(localRequest(server.url("/stall"), 2000, 120));
        QSignalSpy errors(stalled, &QNetworkReply::errorOccurred);
        QSignalSpy finished(stalled, &QNetworkReply::finished);
        QTRY_COMPARE_WITH_TIMEOUT(server.requestCount(), 1, 1000);
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 1000);
        QCOMPARE(stalled->error(), QNetworkReply::TimeoutError);
        QCOMPARE(errors.count(), 1);

        QNetworkReply *recovery = manager.get(localRequest(server.url("/recovery"), 2000, 500));
        QSignalSpy recoveryFinished(recovery, &QNetworkReply::finished);
        QTRY_COMPARE_WITH_TIMEOUT(server.countPath("/recovery"), 1, 1000);
        server.respond(server.socketForPath("/recovery"), 200, "recovered");
        QTRY_COMPARE(recoveryFinished.count(), 1);
        QCOMPARE(recovery->error(), QNetworkReply::NoError);
        QCOMPARE(recovery->readAll(), QByteArray("recovered"));
        delete stalled;
        delete recovery;
    }

    void queueDeadlineExpiresWithoutDispatch() {
        LocalHttpServer server;
        QVERIFY(server.start());
        WikipediaNetwork::NetworkAccessManager managers[4];
        for (auto &manager : managers)
            useDirectTransport(manager);
        QNetworkReply *active[3];
        for (int i = 0; i < 3; ++i)
            active[i] = managers[i].get(localRequest(server.url(QStringLiteral("/hold-%1").arg(i)), 1800, 2500));
        QTRY_COMPARE_WITH_TIMEOUT(server.requestCount(), 3, 1000);

        QNetworkReply *queued = managers[3].get(localRequest(server.url("/must-expire-in-queue"), 180, 2500));
        QSignalSpy queuedErrors(queued, &QNetworkReply::errorOccurred);
        QSignalSpy queuedFinished(queued, &QNetworkReply::finished);
        QTRY_COMPARE_WITH_TIMEOUT(queuedFinished.count(), 1, 800);
        QCOMPARE(queued->error(), QNetworkReply::TimeoutError);
        QCOMPARE(queuedErrors.count(), 1);
        QCOMPARE(server.requestCount(), 3);
        for (auto *reply : active)
            reply->abort();
        delete queued;
        for (auto *reply : active)
            delete reply;
    }

    void operationDeadlineExpiresDuringSlowProgress() {
        LocalHttpServer server;
        QVERIFY(server.start());
        WikipediaNetwork::NetworkAccessManager manager;
        useDirectTransport(manager);
        QNetworkReply *reply = manager.get(localRequest(server.url("/slow-drip"), 260, 700));
        QSignalSpy finished(reply, &QNetworkReply::finished);
        QTRY_COMPARE_WITH_TIMEOUT(server.requestCount(), 1, 1000);
        server.sendSlowBody(server.socketForPath("/slow-drip"));
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 1000);
        QCOMPARE(reply->error(), QNetworkReply::TimeoutError);
        delete reply;
    }

    void numericRetryAfterRetriesThenRecovers() {
        LocalHttpServer server;
        QVERIFY(server.start());
        WikipediaNetwork::NetworkAccessManager manager;
        useDirectTransport(manager);
        const QString path = QStringLiteral("/numeric-retry");
        QNetworkReply *reply = manager.get(localRequest(server.url(path, "127.0.0.1"), 3500, 1000));
        QSignalSpy finished(reply, &QNetworkReply::finished);
        QTRY_COMPARE_WITH_TIMEOUT(server.countPath(path), 1, 1000);
        server.respond(server.socketForPath(path), 429, "retry", {{"Retry-After", "0"}});
        QTRY_COMPARE_WITH_TIMEOUT(server.countPath(path), 2, 2200);
        server.respond(server.socketForPath(path, 1));
        QTRY_COMPARE(finished.count(), 1);
        QCOMPARE(server.countPath(path), 2);
        QCOMPARE(reply->error(), QNetworkReply::NoError);
        delete reply;
    }

    void httpDateRetryAfterAndServiceUnavailableAreHandled() {
        LocalHttpServer server;
        QVERIFY(server.start());
        WikipediaNetwork::NetworkAccessManager manager;
        useDirectTransport(manager);
        const QString path = QStringLiteral("/date-retry");
        QNetworkReply *reply = manager.get(localRequest(server.url(path, "127.0.0.2"), 3500, 1000));
        QSignalSpy finished(reply, &QNetworkReply::finished);
        QTRY_COMPARE_WITH_TIMEOUT(server.countPath(path), 1, 1000);
        const QByteArray pastDate = QDateTime::currentDateTimeUtc().addSecs(-1).toString(Qt::RFC2822Date).toLatin1();
        server.respond(server.socketForPath(path), 503, "retry", {{"Retry-After", pastDate}});
        QTRY_COMPARE_WITH_TIMEOUT(server.countPath(path), 2, 2200);
        server.respond(server.socketForPath(path, 1), 200, "ok");
        QTRY_COMPARE(finished.count(), 1);
        QCOMPARE(reply->error(), QNetworkReply::NoError);
        delete reply;
    }

    void malformedRetryAfterCannotOutliveOperationDeadline() {
        LocalHttpServer server;
        QVERIFY(server.start());
        WikipediaNetwork::NetworkAccessManager manager;
        useDirectTransport(manager);
        const QString path = QStringLiteral("/malformed-retry");
        QNetworkReply *reply = manager.get(localRequest(server.url(path, "127.0.0.3"), 220, 1000));
        QSignalSpy finished(reply, &QNetworkReply::finished);
        QTRY_COMPARE_WITH_TIMEOUT(server.countPath(path), 1, 1000);
        server.respond(server.socketForPath(path), 429, "retry", {{"Retry-After", "tomorrow-ish"}});
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 800);
        QCOMPARE(reply->error(), QNetworkReply::TimeoutError);
        QCOMPARE(server.countPath(path), 1);
        delete reply;
    }

    void retryExhaustionFinishesExactlyOnce() {
        LocalHttpServer server;
        QVERIFY(server.start());
        WikipediaNetwork::NetworkAccessManager manager;
        useDirectTransport(manager);
        const QString path = QStringLiteral("/exhaust-retries");
        QNetworkReply *reply = manager.get(localRequest(server.url(path, "127.0.0.4"), 5000, 1000));
        QSignalSpy errors(reply, &QNetworkReply::errorOccurred);
        QSignalSpy finished(reply, &QNetworkReply::finished);
        for (int attempt = 0; attempt < 3; ++attempt) {
            QTRY_COMPARE_WITH_TIMEOUT(server.countPath(path), attempt + 1, attempt == 0 ? 1000 : 2200);
            server.respond(server.socketForPath(path, attempt), 503, "retry", {{"Retry-After", "0"}});
        }
        QTRY_COMPARE(finished.count(), 1);
        QCOMPARE(errors.count(), 1);
        QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 503);
        QCOMPARE(server.countPath(path), 3);
        delete reply;
    }

    void abortDuringRetryCooldownFinishesExactlyOnce() {
        LocalHttpServer server;
        QVERIFY(server.start());
        WikipediaNetwork::NetworkAccessManager manager;
        useDirectTransport(manager);
        const QString path = QStringLiteral("/abort-retry");
        QNetworkReply *reply = manager.get(localRequest(server.url(path, "127.0.0.7"), 3000, 1000));
        QSignalSpy errors(reply, &QNetworkReply::errorOccurred);
        QSignalSpy finished(reply, &QNetworkReply::finished);
        QTRY_COMPARE_WITH_TIMEOUT(server.countPath(path), 1, 1000);
        server.respond(server.socketForPath(path), 503, "retry", {{"Retry-After", "5"}});
        QTest::qWait(100);
        reply->abort();
        QCOMPARE(finished.count(), 1);
        QCOMPARE(errors.count(), 1);
        QTest::qWait(150);
        QCOMPARE(server.countPath(path), 1);
        delete reply;
    }

    void cooldownIsScopedToOrigin() {
        LocalHttpServer server;
        QVERIFY(server.start());
        LocalHttpServer otherServer;
        QVERIFY(otherServer.start());
        WikipediaNetwork::NetworkAccessManager managers[2];
        for (auto &manager : managers)
            useDirectTransport(manager);
        const QString ratePath = QStringLiteral("/origin-rate-limited");
        QNetworkReply *limited = managers[0].get(localRequest(server.url(ratePath, "127.0.0.5"), 250, 1000));
        QSignalSpy limitedFinished(limited, &QNetworkReply::finished);
        QTRY_COMPARE_WITH_TIMEOUT(server.countPath(ratePath), 1, 1000);
        server.respond(server.socketForPath(ratePath), 429, "retry", {{"Retry-After", "5"}});

        const QString otherPath = QStringLiteral("/other-origin");
        QElapsedTimer elapsed;
        elapsed.start();
        QNetworkReply *other = managers[1].get(localRequest(otherServer.url(otherPath, "127.0.0.5"), 1200, 1000));
        QSignalSpy otherFinished(other, &QNetworkReply::finished);
        QTRY_COMPARE_WITH_TIMEOUT(otherServer.countPath(otherPath), 1, 500);
        QVERIFY2(elapsed.elapsed() < 500, "A host cooldown must not block a different origin");
        otherServer.respond(otherServer.socketForPath(otherPath));
        QTRY_COMPARE(otherFinished.count(), 1);
        QTRY_COMPARE(limitedFinished.count(), 1);
        QCOMPARE(limited->error(), QNetworkReply::TimeoutError);
        delete limited;
        delete other;
    }

    void responseBufferLimitFailsAndReleasesQueueSlot() {
        LocalHttpServer server;
        QVERIFY(server.start());
        WikipediaNetwork::NetworkAccessManager managers[4];
        for (auto &manager : managers)
            useDirectTransport(manager);
        QNetworkReply *active[3];
        for (int i = 0; i < 3; ++i)
            active[i] = managers[i].get(localRequest(server.url(QStringLiteral("/buffer-%1").arg(i)), 4000, 1000));
        QTRY_COMPARE_WITH_TIMEOUT(server.requestCount(), 3, 1000);
        QNetworkRequest limitedRequest = localRequest(server.url("/too-large"), 3000, 1000);
        limitedRequest.setAttribute(MaximumResponseBytesAttribute, 8);
        QNetworkReply *limited = managers[3].get(limitedRequest);
        QSignalSpy errors(limited, &QNetworkReply::errorOccurred);
        QSignalSpy finished(limited, &QNetworkReply::finished);
        delete active[0];
        QTRY_COMPARE_WITH_TIMEOUT(server.countPath("/too-large"), 1, 1000);
        server.respond(server.socketForPath("/too-large"), 200, "123456789");
        QTRY_COMPARE(finished.count(), 1);
        QCOMPARE(errors.count(), 1);
        QCOMPARE(limited->error(), QNetworkReply::ContentOperationNotPermittedError);
        active[1]->abort();
        active[2]->abort();
        delete active[1];
        delete active[2];
        delete limited;
    }
};

QTEST_GUILESS_MAIN(WikipediaNetworkAccessManagerTest)
#include "WikipediaNetworkAccessManagerTest.moc"
