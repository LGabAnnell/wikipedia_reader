#ifndef WIKIPEDIA_NETWORK_ACCESS_MANAGER_H
#define WIKIPEDIA_NETWORK_ACCESS_MANAGER_H

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QQmlNetworkAccessManagerFactory>
#include <QVariant>

namespace WikipediaNetwork {

inline constexpr auto FactoryProperty = "wikipediaNetworkAccessManagerFactory";

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
 * @brief Creates a manager through the installed factory, or a normal Qt manager.
 * @param parent QObject that owns the returned manager.
 */
inline QNetworkAccessManager *createNetworkAccessManager(QObject *parent) {
    auto *application = QCoreApplication::instance();
    auto *factory = application
                        ? static_cast<QQmlNetworkAccessManagerFactory *>(
                              application->property(FactoryProperty).value<void *>())
                        : nullptr;

    return factory ? factory->create(parent) : new QNetworkAccessManager(parent);
}

} // namespace WikipediaNetwork

#endif // WIKIPEDIA_NETWORK_ACCESS_MANAGER_H
