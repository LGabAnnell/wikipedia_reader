#ifndef ARTICLESTATE_H
#define ARTICLESTATE_H

#include "HistoryState.h"
#include "SettingsState.h"
#include "wikipedia_page_client.h"
#include <QMap>
#include <QPointer>
#include <QQmlEngine>

/** @brief Current article, article feedback, cache, and history recording. */
class ArticleState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Singleton")
    Q_PROPERTY(QString currentPageTitle READ currentPageTitle NOTIFY currentPageChanged)
    Q_PROPERTY(QString currentPageExtract READ currentPageExtract NOTIFY currentPageChanged)
    Q_PROPERTY(int currentPageId READ currentPageId NOTIFY currentPageChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

  public:
    /** @brief Dependencies must outlive this application-owned state. */
    explicit ArticleState(HistoryState &history, SettingsState &settings, QObject *parent = nullptr);
    static QPointer<ArticleState> instance() { return m_instance; }
    QString currentPageTitle() const { return m_currentPage.title; }
    QString currentPageExtract() const { return m_currentPage.extract; }
    int currentPageId() const { return m_currentPage.pageid; }
    page currentPage() const { return m_currentPage; }
    QStringList currentPageImageUrls() const { return m_currentPage.imageUrls; }
    bool isLoading() const { return m_isLoading; }
    QString errorMessage() const { return m_errorMessage; }

    /** @brief Load by ID, reusing cached content and recording every visit. */
    Q_INVOKABLE void loadArticleByPageId(int pageId);
    /** @brief Resolve the title, then load the full article. */
    Q_INVOKABLE void loadArticleByTitle(const QString &title);

  public slots:
    void setCurrentPage(const page &article);
    void setCurrentPageFromData(const QString &title, const QString &extract, const QString &url);
    void setIsLoading(bool loading);
    void setErrorMessage(const QString &message);
    void clearErrorMessage();

  signals:
    void currentPageChanged();
    void isLoadingChanged();
    void errorMessageChanged();

  private slots:
    void handleArticleLoadError(const QString &error);

  private:
    static QPointer<ArticleState> m_instance;
    QPointer<HistoryState> m_history;
    QPointer<SettingsState> m_settings;
    WikipediaPageClient *m_pageClient;
    page m_currentPage{};
    QMap<int, page> m_articleCache;
    bool m_isLoading = false;
    QString m_errorMessage;
};

#endif // ARTICLESTATE_H
