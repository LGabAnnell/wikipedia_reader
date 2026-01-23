// src/state/NavigationState.h
#ifndef NAVIGATIONSTATE_H
#define NAVIGATIONSTATE_H

#include <QObject>
#include <QQuickItem>
#include <QEvent>
#include <QMouseEvent>

#include "constants.h"

class NavigationState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QQuickItem* stackView READ stackView WRITE setStackView NOTIFY stackViewChanged)
    Q_PROPERTY(QString currentView READ currentView WRITE setCurrentView NOTIFY currentViewChanged)

public:
    explicit NavigationState(QObject *parent = nullptr);
    QQuickItem* stackView() const;
    QString currentView() const;

    // Install the event filter on a QObject (e.g., QQuickWindow or QQuickItem)
    Q_INVOKABLE void installEventFilter(QObject *obj);

    // Methods to handle navigation
    Q_INVOKABLE void navigateToContent();
    Q_INVOKABLE void navigateToHistory();
    Q_INVOKABLE void navigateToSearch();

    Q_INVOKABLE void navigateToView(const QString &view);
    Q_INVOKABLE void addView(QString view, QQmlComponent *component);

signals:
    void stackViewChanged();
    void currentViewChanged();
    void pushView(const QQmlComponent *view);
    void replaceView(const QQmlComponent *view);
    void backButtonPressed(); // Emitted when the mouse back button is pressed

public slots:
    void setStackView(QQuickItem* stackView);
    void setCurrentView(const QString &view);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QQuickItem* m_stackView;
    QString m_currentView;
    std::map<QString, QQmlComponent*> views;
};

#endif // NAVIGATIONSTATE_H