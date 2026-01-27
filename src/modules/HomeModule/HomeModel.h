// src/modules/HomeModule/HomeModel.h
#ifndef HOMEMODEL_H
#define HOMEMODEL_H

#include <QObject>
#include <QVariantList>
#include <QQmlEngine>
#include <QJSEngine>
#include "wikipedia_client.h"

class HomeModel : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(HomeModel)
    // Featured article properties
    Q_PROPERTY(QString featuredArticleTitle READ featuredArticleTitle NOTIFY featuredArticleUpdated)
    Q_PROPERTY(QString featuredArticleExtract READ featuredArticleExtract NOTIFY featuredArticleUpdated)
    Q_PROPERTY(QString featuredArticleImageUrl READ featuredArticleImageUrl NOTIFY featuredArticleImageUpdated)
    Q_PROPERTY(QString featuredArticleUrl READ featuredArticleUrl NOTIFY featuredArticleUpdated)

    // In the news properties
    Q_PROPERTY(QVariantList newsItems READ newsItems NOTIFY dataUpdated)

    // On this day properties
    Q_PROPERTY(QVariantList onThisDayEvents READ onThisDayEvents NOTIFY dataUpdated)

    // Did you know properties
    Q_PROPERTY(QVariantList didYouKnowItems READ didYouKnowItems NOTIFY dataUpdated)

public:
    explicit HomeModel(QObject *parent = nullptr);

    // Singleton instance creator for QML
    static HomeModel* create(QQmlEngine *engine, QJSEngine *scriptEngine);

    // Getters for QML properties
    Q_INVOKABLE QString featuredArticleTitle() const;
    Q_INVOKABLE QString featuredArticleExtract() const;
    Q_INVOKABLE QString featuredArticleImageUrl() const;
    Q_INVOKABLE QString featuredArticleUrl() const;
    Q_INVOKABLE QVariantList newsItems() const;
    Q_INVOKABLE QVariantList onThisDayEvents() const;
    Q_INVOKABLE QVariantList didYouKnowItems() const;

    // Fetch data from Wikipedia
    Q_INVOKABLE void fetchHomeData();

signals:
    void featuredArticleUpdated();
    void featuredArticleImageUpdated();
    void dataUpdated();

private slots:
    void handleFeaturedArticleReceived(const QString &title, const QString &extract, const int &pageid);
    void handlePageWithImagesReceived(const page &page);
    void handleError(const QString &error);
    void handleNewsItemsReceived(const QVector<news_item> &items);
    void handleOnThisDayEventsReceived(const QVector<on_this_day_event> &events);
    void handleDidYouKnowItemsReceived(const QVector<did_you_know_item> &items);

private:
    WikipediaClient *m_wikipediaClient;

    // Data members
    QString m_featuredArticleTitle;
    QString m_featuredArticleExtract;
    QString m_featuredArticleImageUrl;
    QString m_featuredArticleUrl;
    QVariantList m_newsItems;
    QVariantList m_onThisDayEvents;
    QVariantList m_didYouKnowItems;
};

#endif // HOMEMODEL_H