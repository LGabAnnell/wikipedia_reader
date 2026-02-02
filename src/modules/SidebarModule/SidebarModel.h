#ifndef SIDEBARMODEL_H
#define SIDEBARMODEL_H

#include <QObject>
#include <QString>
#include <QQmlEngine>
#include <QVector>
#include "wikipedia_models.h"

class SidebarModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString currentView READ currentView WRITE setCurrentView NOTIFY currentViewChanged)
    Q_PROPERTY(QVector<search_result> searchResults READ searchResults WRITE setSearchResults NOTIFY searchResultsChanged)

public:
    explicit SidebarModel(QObject *parent = nullptr);

    QString currentView() const;
    void setCurrentView(const QString &view);

    QVector<search_result> searchResults() const;
    void setSearchResults(const QVector<search_result> &results);

signals:
    void currentViewChanged(const QString &view);
    void navigateTo(const QString &view);
    void searchResultsChanged();

private:
    QString m_currentView;
    QVector<search_result> m_searchResults;
};

#endif // SIDEBARMODEL_H
