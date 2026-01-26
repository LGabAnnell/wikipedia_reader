// src/state/NavigationState.cpp
#include "NavigationState.h"
#include <QDebug>
#include <QQuickItem>
#include <QEvent>
#include <QMouseEvent>

NavigationState::NavigationState(QObject *parent)
    : QObject(parent), m_stackView(nullptr) {}

QQuickItem* NavigationState::stackView() const {
    return m_stackView;
}

void NavigationState::setStackView(QQuickItem* stackView) {
    if (m_stackView != stackView) {
        m_stackView = stackView;
        emit stackViewChanged();
    }
}

void NavigationState::setCurrentView(const QString &view) {
    emit replaceView(views.at(view));
}

void NavigationState::installEventFilter(QObject *obj) {
    if (obj) {
        obj->installEventFilter(this);
    }
}

bool NavigationState::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::BackButton) {
            emit backButtonPressed();
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

void NavigationState::navigateToContent() {
    if (!m_stackView) {
        qWarning() << "StackView is not set!";
        return;
    }

    // Emit signal to indicate navigation to content
    navigateToView(Constants::contentView());
}

void NavigationState::navigateToHistory() {
    if (!m_stackView) {
        qWarning() << "StackView is not set!";
        return;
    }

    // Emit signal to indicate navigation to history
    navigateToView(Constants::historyView());
}

void NavigationState::navigateToSearch() {
    if (!m_stackView) {
        qWarning() << "StackView is not set!";
        return;
    }

    // Emit signal to indicate navigation to search
    navigateToView(Constants::searchView());
}

void NavigationState::navigateToView(const QString &view) {
    emit pushView(views.at(view));
}

void NavigationState::addView(QString view, QQmlComponent *component) {
    views.emplace(view, component);
}
