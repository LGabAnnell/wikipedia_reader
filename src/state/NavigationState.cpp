// src/state/NavigationState.cpp
#include "NavigationState.h"
#include <QDebug>
#include <QEvent>
#include <QMouseEvent>
#include <QQuickItem>

NavigationState::NavigationState(QObject *parent) : QObject(parent), m_stackView(nullptr) {}

QQuickItem *NavigationState::stackView() const { return m_stackView; }

void NavigationState::setStackView(QQuickItem *stackView) {
    if (m_stackView != stackView) {
        m_stackView = stackView;
        emit stackViewChanged();
    }
}

void NavigationState::installEventFilter(QObject *obj) {
    if (obj) {
        obj->installEventFilter(this);
    }
}

bool NavigationState::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::BackButton) {
            emit backButtonPressed();
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

void NavigationState::navigateToContent(const QString &initialFragment) {
    if (!m_stackView) {
        qWarning() << "StackView is not set!";
        return;
    }

    QVariantMap properties;
    if (!initialFragment.isEmpty())
        properties.insert(QStringLiteral("initialFragmentAnchor"), initialFragment);
    pushRegisteredView(Constants::contentView(), properties);
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

void NavigationState::navigateToImageGallery() {
    if (!m_stackView) {
        qWarning() << "StackView is not set!";
        return;
    }

    // Emit signal to indicate navigation to image gallery
    navigateToView(Constants::imageGalleryView());
}

void NavigationState::navigateToView(const QString &view) {
    pushRegisteredView(view, {});
}

void NavigationState::pushRegisteredView(const QString &view, const QVariantMap &properties) {
    const auto it = views.find(view);
    if (it == views.end()) {
        qWarning() << "Cannot navigate to unregistered view:" << view;
        return;
    }

    // Header actions and callers may request history more than once. Keep a
    // single history view at the top while still allowing it to be pushed
    // again when another view is currently visible.
    if (view == Constants::historyView() && m_stackView) {
        QObject *currentItem = m_stackView->property("currentItem").value<QObject *>();
        if (currentItem && currentItem->objectName() == QStringLiteral("historyView")) {
            return;
        }
    }

    emit pushView(it->second, properties);
}

void NavigationState::addView(QString view, QQmlComponent *component) { views.emplace(view, component); }
