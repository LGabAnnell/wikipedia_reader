#ifndef SECTIONMODEL_H
#define SECTIONMODEL_H

#include <QObject>
#include <QString>
#include <QQmlEngine>
#include <QVector>
#include "wikipedia_models.h"

class WikipediaPageClient;

class SectionModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    
    Q_PROPERTY(QVector<section> sections READ sections NOTIFY sectionsChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)

public:
    explicit SectionModel(QObject *parent = nullptr);
    
    QVector<section> sections() const;
    bool isLoading() const;
    QString errorMessage() const;
    
    Q_INVOKABLE void fetchSections(const QString &title);
    Q_INVOKABLE void clearSections();

signals:
    void sectionsChanged();
    void loadingChanged();
    void errorChanged();

private slots:
    void handleSectionsReceived(const QVector<section> &sections);
    void handleError(const QString &error);

private:
    QVector<section> m_sections;
    bool m_isLoading;
    QString m_errorMessage;
    WikipediaPageClient* m_pageClient;
};

#endif // SECTIONMODEL_H

