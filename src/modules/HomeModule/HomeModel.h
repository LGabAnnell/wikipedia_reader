// src/modules/HomeModule/HomeModel.h
#ifndef HOMEMODEL_H
#define HOMEMODEL_H

#include <QObject>
#include <QVariantList>
#include <QQmlEngine>
#include <QJSEngine>
#include <QPointer>
#include "wikipedia_client.h"

class HomeModel : public QObject
{
    Q_OBJECT
        QML_NAMED_ELEMENT(HomeModel)
        // Featured article properties
        Q_PROPERTY(QString featuredArticleTitle READ featuredArticleTitle NOTIFY featuredArticleUpdated)
        Q_PROPERTY(QString featuredArticleExtract READ featuredArticleExtract NOTIFY featuredArticleUpdated)
        Q_PROPERTY(QString featuredArticleImageUrl READ featuredArticleImageUrl NOTIFY featuredArticleImageUpdated)
        Q_PROPERTY(QString featuredArticleUrl READ featuredArticleUrl NOTIFY featuredArticleUpdated)

        // In the news properties (QVariantList for QML compatibility)
        Q_PROPERTY(QVector<news_item> newsItems READ newsItems NOTIFY dataUpdated)

        // On this day properties (QVariantList for QML compatibility)
        Q_PROPERTY(QVector<on_this_day_event> onThisDayEvents READ onThisDayEvents NOTIFY dataUpdated)

        // Did you know properties (QVariantList for QML compatibility)
        Q_PROPERTY(QVector<did_you_know_item> didYouKnowItems READ didYouKnowItems NOTIFY dataUpdated)

public:
    explicit HomeModel(QObject *parent = nullptr);

    // Getters for QML properties (QVariantList for QML compatibility)
    Q_INVOKABLE QString featuredArticleTitle() const;
    Q_INVOKABLE QString featuredArticleExtract() const;
    Q_INVOKABLE QString featuredArticleImageUrl() const;
    Q_INVOKABLE QString featuredArticleUrl() const;

    Q_INVOKABLE QVector<news_item> newsItems() const {
        return m_newsItems;
    }

    Q_INVOKABLE QVector<on_this_day_event> onThisDayEvents() const {
        return m_onThisDayEvents;
    }

    Q_INVOKABLE QVector<did_you_know_item> didYouKnowItems() const {
        return m_didYouKnowItems;
    }

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
    QPointer<WikipediaClient> m_wikipediaClient;

    // Data members
    QString m_featuredArticleTitle;
    QString m_featuredArticleExtract;
    QString m_featuredArticleImageUrl;
    QString m_featuredArticleUrl;

    // Strongly-typed data members
    QVector<news_item> m_newsItems;
    QVector<on_this_day_event> m_onThisDayEvents;
    QVector<did_you_know_item> m_didYouKnowItems;
};

#endif // HOMEMODEL_H