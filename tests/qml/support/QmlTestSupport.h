#ifndef QML_TEST_SUPPORT_H
#define QML_TEST_SUPPORT_H

#include "FakeNetworkAccessManager.h"

#include <QQmlError>
#include <QObject>
#include <QStringList>

class QmlTestSupport final : public QObject {
    Q_OBJECT
    Q_PROPERTY(int qmlWarningCount READ qmlWarningCount NOTIFY qmlWarningsChanged)
    Q_PROPERTY(QStringList qmlWarnings READ qmlWarnings NOTIFY qmlWarningsChanged)
    Q_PROPERTY(int requestCount READ requestCount NOTIFY requestsChanged)
    Q_PROPERTY(QVariantList requests READ requests NOTIFY requestsChanged)

  public:
    explicit QmlTestSupport(NetworkFixtureController *networkController, QObject *parent = nullptr);

    int qmlWarningCount() const;
    QStringList qmlWarnings() const;
    int requestCount() const;
    QVariantList requests() const;
    Q_INVOKABLE void clearQmlWarnings();
    Q_INVOKABLE void clearRequests();

  public slots:
    void recordQmlWarnings(const QList<QQmlError> &warnings);

  signals:
    void qmlWarningsChanged();
    void requestsChanged();

  private:
    NetworkFixtureController *m_networkController;
    QStringList m_qmlWarnings;
};

#endif // QML_TEST_SUPPORT_H
