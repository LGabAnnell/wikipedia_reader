#ifndef FAKE_NETWORK_ACCESS_MANAGER_H
#define FAKE_NETWORK_ACCESS_MANAGER_H

#include "FakeNetworkReply.h"

#include <QQmlNetworkAccessManagerFactory>
#include <QHash>
#include <QMutex>
#include <QNetworkAccessManager>
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

  public:
    explicit NetworkFixtureController(QObject *parent = nullptr);

    QNetworkAccessManager *create(QObject *parent) override;
    int requestCount() const;
    QVariantList requests() const;

    Q_INVOKABLE void addFixture(const QString &method, const QUrl &url, const QByteArray &body,
                                const QString &contentType = QStringLiteral("application/json"),
                                int statusCode = 200, int networkError = QNetworkReply::NoError,
                                const QString &errorString = QString());
    Q_INVOKABLE void clearRequests();

  signals:
    void requestsChanged();
    void unexpectedRequest(const QString &description);

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
    mutable QMutex m_mutex;
};

#endif // FAKE_NETWORK_ACCESS_MANAGER_H
