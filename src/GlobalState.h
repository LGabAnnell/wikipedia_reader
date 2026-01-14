// src/GlobalState.h
#ifndef GLOBALSTATE_H
#define GLOBALSTATE_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QQmlEngine>
#include "wikipedia_client/wikipedia_client.h"

class GlobalState : public QObject {
    Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

        // Expose Page properties directly
        Q_PROPERTY(QVector<Wikipedia::SearchResult> searchResults READ searchResults NOTIFY searchResultsChanged)
        Q_PROPERTY(QString currentPageTitle READ currentPageTitle NOTIFY currentPageChanged)
        Q_PROPERTY(QString currentPageExtract READ currentPageExtract NOTIFY currentPageChanged)
        Q_PROPERTY(int currentPageId READ currentPageId NOTIFY currentPageChanged)
        Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
        // Add error message property
        Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit GlobalState(QObject *parent = nullptr);

    QVector<Wikipedia::SearchResult> searchResults() const;

    // Page property accessors
    QString currentPageTitle() const;
    QString currentPageExtract() const;
    int currentPageId() const;

    bool isLoading() const;
    QString errorMessage() const;

public slots:
    void setSearchResults(const QVector<Wikipedia::SearchResult> &results);
    void setCurrentPage(const Wikipedia::Page &page);
    void setIsLoading(bool loading);
    void setErrorMessage(const QString &message);
    void clearErrorMessage();

signals:
    void searchResultsChanged();
    void currentPageChanged();
    void isLoadingChanged();
    void errorMessageChanged();

private:
    QVector<Wikipedia::SearchResult> m_searchResults;
    Wikipedia::Page m_currentPage;
    bool m_isLoading;
    QString m_errorMessage;
};

#endif // GLOBALSTATE_H