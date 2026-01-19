// SidebarModel.cpp
#include "SidebarModel.h"
#include <QDebug>

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

QVector<search_result> SidebarModel::searchResults() const {
    return m_searchResults;
}

void SidebarModel::setSearchResults(const QVector<search_result> &results) {
    m_searchResults = results;
    qDebug() << "Got " << results.length() << " elements";
    emit searchResultsChanged();
}
