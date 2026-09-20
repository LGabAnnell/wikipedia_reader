#ifndef FAKE_NETWORK_ACCESS_MANAGER_H
#define FAKE_NETWORK_ACCESS_MANAGER_H

#include "FakeNetworkReply.h"

#include <QQmlNetworkAccessManagerFactory>
#include <QHash>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QQueue>
#include <QVariantList>

class NetworkFixtureController;

class FakeNetworkAccessManager final : public QNetworkAccessManager {
    Q_OBJECT

  public:
    FakeNetworkAccessManager(NetworkFixtureController *controller, QObject *parent = nullptr);

  protected:
    QNetworkReply *createRequest(Operation operation, const QNetworkRequest &request,
                                 QIODevice *outgoingData = nullptr) override;

  private:
    NetworkFixtureController *m_controller;
};

class NetworkFixtureController final : public QObject, public QQmlNetworkAccessManagerFactory {
    Q_OBJECT
    Q_PROPERTY(int requestCount READ requestCount NOTIFY requestsChanged)
    Q_PROPERTY(QVariantList requests READ requests NOTIFY requestsChanged)
    Q_PROPERTY(int pendingReplyCount READ pendingReplyCount NOTIFY pendingRepliesChanged)

  public:
    explicit NetworkFixtureController(QObject *parent = nullptr);

    QNetworkAccessManager *create(QObject *parent) override;
    int requestCount() const;
    QVariantList requests() const;
    int pendingReplyCount() const;

    Q_INVOKABLE void addFixture(const QString &method, const QUrl &url, const QByteArray &body,
                                const QString &contentType = QStringLiteral("application/json"),
                                int statusCode = 200, int networkError = QNetworkReply::NoError,
                                const QString &errorString = QString());
    Q_INVOKABLE void clearRequests();
    Q_INVOKABLE void deferNextReply();
    Q_INVOKABLE void completeNextReply();

  signals:
    void requestsChanged();
    void unexpectedRequest(const QString &description);
    void pendingRepliesChanged();

  private:
    friend class FakeNetworkAccessManager;

    static QString requestKey(const QString &method, const QUrl &url);
    static QByteArray methodForOperation(QNetworkAccessManager::Operation operation,
                                         const QNetworkRequest &request);
    FakeNetworkReply *handleRequest(QNetworkAccessManager::Operation operation,
                                    const QNetworkRequest &request, QObject *parent);
    void addFixtureFile(const QString &method, const QUrl &url, const QString &fixtureFile,
                        const QString &contentType);

    QHash<QString, FakeNetworkResponse> m_fixtures;
    QVariantList m_requests;
    QQueue<QPointer<FakeNetworkReply>> m_pendingReplies;
    bool m_deferNextReply = false;
    mutable QMutex m_mutex;
};

#endif // FAKE_NETWORK_ACCESS_MANAGER_H
