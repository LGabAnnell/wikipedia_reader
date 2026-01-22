// src/NavigationState.cpp
#include "NavigationState.h"
#include <QDebug>
#include <QMouseEvent>

NavigationState::NavigationState(QObject *parent)
    : QObject(parent), m_stackView(nullptr) {
}

QQuickItem* NavigationState::stackView() const {
    return m_stackView;
}

void NavigationState::setStackView(QQuickItem* stackView) {
    if (m_stackView != stackView) {
        m_stackView = stackView;
        emit stackViewChanged();
    }
}

void NavigationState::navigateToContent() {
    emit navigateToContentRequested();
}

void NavigationState::navigateToHistory() {
    emit navigateToHistoryRequested();
}

void NavigationState::navigateToSearch() {
    emit navigateToSearchRequested();
}

// Install the event filter on the provided QObject
void NavigationState::installEventFilter(QObject *obj) {
    if (obj) {
        obj->installEventFilter(this);
    }
}

// Override eventFilter to intercept mouse back button events
bool NavigationState::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::BackButton) {
            emit backButtonPressed();
            return true; // Event handled
        }
    }
    // Pass the event on to the parent class
    return QObject::eventFilter(obj, event);
}
