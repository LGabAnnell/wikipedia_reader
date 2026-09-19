#include "QmlTestSupport.h"

QmlTestSupport::QmlTestSupport(NetworkFixtureController *networkController, QObject *parent)
    : QObject(parent), m_networkController(networkController) {
    connect(m_networkController, &NetworkFixtureController::requestsChanged, this,
            &QmlTestSupport::requestsChanged);
}

int QmlTestSupport::qmlWarningCount() const { return m_qmlWarnings.size(); }

QStringList QmlTestSupport::qmlWarnings() const { return m_qmlWarnings; }

int QmlTestSupport::requestCount() const { return m_networkController->requestCount(); }

QVariantList QmlTestSupport::requests() const { return m_networkController->requests(); }

void QmlTestSupport::clearQmlWarnings() {
    if (m_qmlWarnings.isEmpty()) {
        return;
    }
    m_qmlWarnings.clear();
    emit qmlWarningsChanged();
}

void QmlTestSupport::clearRequests() { m_networkController->clearRequests(); }

void QmlTestSupport::recordQmlWarnings(const QList<QQmlError> &warnings) {
    for (const QQmlError &warning : warnings) {
        m_qmlWarnings.append(warning.toString());
    }
    if (!warnings.isEmpty()) {
        emit qmlWarningsChanged();
    }
}
