#ifndef SIDEBARMODEL_H
#define SIDEBARMODEL_H

#include <QObject>
#include <QString>
#include <QQmlEngine>
#include <QVector>
#include "wikipedia_client.h"

class SidebarModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString currentView READ currentView WRITE setCurrentView NOTIFY currentViewChanged)
    Q_PROPERTY(QVector<SearchResult> searchResults READ searchResults WRITE setSearchResults NOTIFY searchResultsChanged)

public:
    explicit SidebarModel(QObject *parent = nullptr);

    QString currentView() const;
    void setCurrentView(const QString &view);

    QVector<SearchResult> searchResults() const;
    void setSearchResults(const QVector<SearchResult> &results);

signals:
    void currentViewChanged(const QString &view);
    void navigateTo(const QString &view);
    void searchResultsChanged();

public slots:
    void navigateToHome();
    void navigateToHistory();
    void navigateToBookmarks();

private:
    QString m_currentView;
    QVector<SearchResult> m_searchResults;
};

#endif // SIDEBARMODEL_H
