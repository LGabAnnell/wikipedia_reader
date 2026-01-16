// SidebarModel.cpp
#include "SidebarModel.h"

SidebarModel::SidebarModel(QObject *parent) : QObject(parent) {
    m_currentView = "home";
}

QString SidebarModel::currentView() const {
    return m_currentView;
}

void SidebarModel::setCurrentView(const QString &view) {
    if (m_currentView != view) {
        m_currentView = view;
        emit currentViewChanged(m_currentView);
    }
}

void SidebarModel::navigateToHome() {
    setCurrentView("home");
    emit navigateTo("home");
}

void SidebarModel::navigateToHistory() {
    setCurrentView("history");
    emit navigateTo("history");
}

void SidebarModel::navigateToBookmarks() {
    setCurrentView("bookmarks");
    emit navigateTo("bookmarks");
}
