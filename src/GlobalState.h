// src/GlobalState.h
#ifndef GLOBALSTATE_H
#define GLOBALSTATE_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QQmlEngine>
#include "wikipedia_client.h"

class GlobalState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    QML_UNCREATABLE("Singleton")

    // Expose Page properties directly
    Q_PROPERTY(QVector<SearchResult> searchResults READ searchResults NOTIFY searchResultsChanged)
    Q_PROPERTY(QString currentPageTitle READ currentPageTitle NOTIFY currentPageChanged)
    Q_PROPERTY(QString currentPageExtract READ currentPageExtract NOTIFY currentPageChanged)
    Q_PROPERTY(int currentPageId READ currentPageId NOTIFY currentPageChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

    // Add error message property
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    Q_INVOKABLE void loadArticleByPageId(int pageId);
    explicit GlobalState(QObject *parent = nullptr);

    // Page property accessors
    QString currentPageTitle() const;
    QString currentPageExtract() const;
    int currentPageId() const;
    QVector<SearchResult> searchResults() const;

    bool isLoading() const;
    QString errorMessage() const;

    static QPointer<GlobalState> instance() {
        return m_instance;
    }

public slots:
    void setSearchResults(const QVector<SearchResult> &results);
    void setCurrentPage(const Page &page);
    void setIsLoading(bool loading);
    void setErrorMessage(const QString &message);
    void clearErrorMessage();

signals:
    void searchResultsChanged();
    void currentPageChanged();
    void isLoadingChanged();
    void errorMessageChanged();

private:
    QVector<SearchResult> m_searchResults;
    Page m_currentPage;
    bool m_isLoading;
    QString m_errorMessage;
    static QPointer<GlobalState> m_instance;
    WikipediaClient* m_wikipediaClient;

private slots:
    void handleArticleLoadError(const QString &error);
};

#endif // GLOBALSTATE_H
