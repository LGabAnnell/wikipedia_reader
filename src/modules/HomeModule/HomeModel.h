#ifndef HOMEMODEL_H
#define HOMEMODEL_H

#include "SettingsState.h"
#include "wikipedia_featured_client.h"
#include "wikipedia_home_client.h"
#include "wikipedia_models.h"
#include <QObject>
#include <QPointer>

class HomeModel : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(HomeModel)
    Q_PROPERTY(QString featuredArticleTitle READ featuredArticleTitle NOTIFY featuredArticleUpdated)
    Q_PROPERTY(QString featuredArticleExtract READ featuredArticleExtract NOTIFY featuredArticleUpdated)
    Q_PROPERTY(QString featuredArticleImageUrl READ featuredArticleImageUrl NOTIFY featuredArticleUpdated)
    Q_PROPERTY(QString featuredArticleUrl READ featuredArticleUrl NOTIFY featuredArticleUpdated)
    Q_PROPERTY(int featuredArticlePageId READ featuredArticlePageId NOTIFY featuredArticleUpdated)
    Q_PROPERTY(QVector<news_item> newsItems READ newsItems NOTIFY dataUpdated)
    Q_PROPERTY(QVector<on_this_day_event> onThisDayEvents READ onThisDayEvents NOTIFY dataUpdated)
    Q_PROPERTY(QVector<did_you_know_item> didYouKnowItems READ didYouKnowItems NOTIFY dataUpdated)
    Q_PROPERTY(Status featuredStatus READ featuredStatus NOTIFY statusChanged)
    Q_PROPERTY(Status newsStatus READ newsStatus NOTIFY statusChanged)
    Q_PROPERTY(Status onThisDayStatus READ onThisDayStatus NOTIFY statusChanged)
    Q_PROPERTY(Status didYouKnowStatus READ didYouKnowStatus NOTIFY statusChanged)
    Q_PROPERTY(QString featuredError READ featuredError NOTIFY statusChanged)
    Q_PROPERTY(QString newsError READ newsError NOTIFY statusChanged)
    Q_PROPERTY(QString onThisDayError READ onThisDayError NOTIFY statusChanged)
    Q_PROPERTY(QString didYouKnowError READ didYouKnowError NOTIFY statusChanged)
  public:
    enum Section { Featured, News, OnThisDay, DidYouKnow };
    Q_ENUM(Section)
    enum Status { Idle, Loading, Ready, Empty, Error };
    Q_ENUM(Status)
    explicit HomeModel(QObject *parent = nullptr);
    QString featuredArticleTitle() const { return m_featuredArticleTitle; }
    QString featuredArticleExtract() const { return m_featuredArticleExtract; }
    QString featuredArticleImageUrl() const { return m_featuredArticleImageUrl; }
    QString featuredArticleUrl() const { return m_featuredArticleUrl; }
    int featuredArticlePageId() const { return m_featuredArticlePageId; }
    QVector<news_item> newsItems() const { return m_newsItems; }
    QVector<on_this_day_event> onThisDayEvents() const { return m_onThisDayEvents; }
    QVector<did_you_know_item> didYouKnowItems() const { return m_didYouKnowItems; }
    Status featuredStatus() const { return m_status[Featured]; }
    Status newsStatus() const { return m_status[News]; }
    Status onThisDayStatus() const { return m_status[OnThisDay]; }
    Status didYouKnowStatus() const { return m_status[DidYouKnow]; }
    QString featuredError() const { return m_errors[Featured]; }
    QString newsError() const { return m_errors[News]; }
    QString onThisDayError() const { return m_errors[OnThisDay]; }
    QString didYouKnowError() const { return m_errors[DidYouKnow]; }
    /** @brief Cancel and reload all four home feeds. */
    Q_INVOKABLE void fetchHomeData();
    /** @brief Retry one failed or empty section without reloading other feeds. */
    Q_INVOKABLE void retrySection(Section section);
  signals:
    void featuredArticleUpdated();
    void dataUpdated();
    void statusChanged();

  private:
    void loadSection(Section section);
    void clearSection(Section section);
    void setStatus(Section section, Status status, const QString &error = {});
    void handleHomeError(WikipediaHomeClient::Section section, const QString &error);
    QPointer<SettingsState> m_settings;
    QPointer<WikipediaFeaturedClient> m_featuredClient;
    QPointer<WikipediaHomeClient> m_homeClient;
    QString m_featuredArticleTitle, m_featuredArticleExtract, m_featuredArticleImageUrl, m_featuredArticleUrl;
    int m_featuredArticlePageId = 0;
    QVector<news_item> m_newsItems;
    QVector<on_this_day_event> m_onThisDayEvents;
    QVector<did_you_know_item> m_didYouKnowItems;
    Status m_status[4] = {Idle, Idle, Idle, Idle};
    QString m_errors[4];
};

#endif
