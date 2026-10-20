#ifndef WIKIPEDIA_NETWORK_ACCESS_MANAGER_H
#define WIKIPEDIA_NETWORK_ACCESS_MANAGER_H

#include <QCoreApplication>
#include <QLibraryInfo>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QQmlNetworkAccessManagerFactory>
#include <QVariant>
#include <memory>
#include "wikipedia_queued_reply.h"
#include <QVersionNumber>

namespace WikipediaNetwork {

inline constexpr auto FactoryProperty = "wikipediaNetworkAccessManagerFactory";

/** @brief Identifies the app to Wikimedia and applies runtime compatibility fixes. */
class NetworkAccessManager final : public QNetworkAccessManager {
  public:
    explicit NetworkAccessManager(QObject *parent = nullptr) : QNetworkAccessManager(parent) {}

  protected:
    QNetworkReply *createRequest(Operation operation, const QNetworkRequest &request,
                                QIODevice *outgoingData = nullptr) override {
        QNetworkRequest compatibleRequest(request);
        // Wikimedia expects an app name/version and a contact URL, followed by
        // the framework version: https://foundation.wikimedia.org/wiki/Policy:Wikimedia_Foundation_User-Agent_Policy
        compatibleRequest.setRawHeader("User-Agent",
                                       QByteArrayLiteral("WikipediaReader/" WIKIPEDIA_READER_VERSION
                                                         " (https://github.com/LGabAnnell/wikipedia_reader) Qt/")
                                           + qVersion());
        // Qt 6.11.2's HTTP/2 frame reader can read a closed TLS socket when an
        // idle connection closes. Use HTTP/1.1 on the affected runtime only.
        if (QLibraryInfo::version() == QVersionNumber(6, 11, 2)) {
            compatibleRequest.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
        }
        if (operation == GetOperation) {
            return new QueuedReply(compatibleRequest, this, m_queue, [this, compatibleRequest] {
                return QNetworkAccessManager::createRequest(GetOperation, compatibleRequest, nullptr);
            });
        }
        return QNetworkAccessManager::createRequest(operation, compatibleRequest, outgoingData);
    }
  private:
    static std::shared_ptr<DownloadQueue> requestQueue() {
        static auto queue = std::make_shared<DownloadQueue>();
        return queue;
    }
    std::shared_ptr<DownloadQueue> m_queue = requestQueue();
};

/**
 * @brief Installs the process-wide manager factory used by Wikipedia clients.
 * @param factory Non-owning factory pointer. Its owner must outlive every client
 * and QQmlEngine that uses it. Install the factory before constructing states,
 * models, or QML engines.
 */
inline void installNetworkAccessManagerFactory(QQmlNetworkAccessManagerFactory *factory) {
    if (QCoreApplication::instance()) {
        QCoreApplication::instance()->setProperty(FactoryProperty, QVariant::fromValue(static_cast<void *>(factory)));
    }
}

/**
 * @brief Creates a manager through the installed factory, or the runtime policy manager.
 * @param parent QObject that owns the returned manager.
 */
inline QNetworkAccessManager *createNetworkAccessManager(QObject *parent) {
    auto *application = QCoreApplication::instance();
    auto *factory = application
                        ? static_cast<QQmlNetworkAccessManagerFactory *>(
                              application->property(FactoryProperty).value<void *>())
                        : nullptr;

    return factory ? factory->create(parent) : new NetworkAccessManager(parent);
}

/** @brief Applies the same network policy to QML images and Wikipedia clients. */
class NetworkAccessManagerFactory final : public QQmlNetworkAccessManagerFactory {
  public:
    QNetworkAccessManager *create(QObject *parent) override {
        return new NetworkAccessManager(parent);
    }
};

} // namespace WikipediaNetwork

#endif // WIKIPEDIA_NETWORK_ACCESS_MANAGER_H
