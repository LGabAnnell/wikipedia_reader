// src/NavigationState.h
#ifndef NAVIGATIONSTATE_H
#define NAVIGATIONSTATE_H

#include <QObject>
#include <QQuickItem>
#include <QEvent>
#include <QMouseEvent>

class NavigationState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QQuickItem* stackView READ stackView WRITE setStackView NOTIFY stackViewChanged)

public:
    explicit NavigationState(QObject *parent = nullptr);
    QQuickItem* stackView() const;

    // Install the event filter on a QObject (e.g., QQuickWindow or QQuickItem)
    Q_INVOKABLE void installEventFilter(QObject *obj);

signals:
    void stackViewChanged();
    void navigateToContentRequested();
    void navigateToHistoryRequested();
    void navigateToSearchRequested();
    void backButtonPressed(); // Emitted when the mouse back button is pressed

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

public slots:
    void setStackView(QQuickItem* stackView);
    void navigateToContent();
    void navigateToHistory();
    void navigateToSearch();

private:
    QQuickItem* m_stackView;
};

#endif // NAVIGATIONSTATE_H
