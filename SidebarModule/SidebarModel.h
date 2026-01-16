#ifndef SIDEBARMODEL_H
#define SIDEBARMODEL_H

#include <QObject>
#include <QString>
#include <QQmlEngine>

class SidebarModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString currentView READ currentView WRITE setCurrentView NOTIFY currentViewChanged)

public:
    explicit SidebarModel(QObject *parent = nullptr);

    QString currentView() const;
    void setCurrentView(const QString &view);

signals:
    void currentViewChanged(const QString &view);
    void navigateTo(const QString &view);

public slots:
    void navigateToHome();
    void navigateToHistory();
    void navigateToBookmarks();

private:
    QString m_currentView;
};

#endif // SIDEBARMODEL_H
