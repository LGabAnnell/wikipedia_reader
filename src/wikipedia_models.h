// wikipedia_models.h
#ifndef WIKIPEDIA_MODELS_H
#define WIKIPEDIA_MODELS_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QQmlEngine>

// Data structure for search results
class search_result
{
    Q_GADGET
    QML_ELEMENT
    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString snippet MEMBER snippet)
    Q_PROPERTY(int pageid MEMBER pageid)
public:
    QString title;
    QString snippet;
    int pageid;
};

// Data structure for a Wikipedia page
class page
{
    Q_GADGET
    QML_VALUE_TYPE(page)
    QML_ELEMENT
    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString extract MEMBER extract)
    Q_PROPERTY(int pageid MEMBER pageid)
    Q_PROPERTY(QStringList imageUrls MEMBER imageUrls)
public:
    QString title;
    QString extract;
    int pageid;
    QStringList imageUrls;
};

// Data structure for a featured article
class featured_article
{
    Q_GADGET
    QML_ELEMENT
    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString extract MEMBER extract)
    Q_PROPERTY(int pageid MEMBER pageid)
    Q_PROPERTY(QStringList imageUrls MEMBER imageUrls)
public:
    QString title;
    QString extract;
    int pageid;
    QStringList imageUrls;
    QString date;
};

// Data structure for a news item
class news_item
{
    Q_GADGET
    QML_ELEMENT
    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString imageUrl MEMBER imageUrl)
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QString url MEMBER url)
public:
    QString title;
    QString imageUrl;
    QString description;
    QString url;
};

// Data structure for an "On This Day" event
class on_this_day_event
{
    Q_GADGET
    QML_ELEMENT
    Q_PROPERTY(int year MEMBER year)
    Q_PROPERTY(QString event MEMBER event)
    Q_PROPERTY(QString url MEMBER url)
public:
    int year;
    QString event;
    QString url;
};

// Data structure for a "Did You Know" item
class did_you_know_item
{
    Q_GADGET
    QML_ELEMENT
    Q_PROPERTY(QString text MEMBER text)
    Q_PROPERTY(QString url MEMBER url)
public:
    QString text;
    QString url;
};

// Data structure for a history item
class history_item
{
    Q_GADGET
    QML_ELEMENT
    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(int pageId MEMBER pageId)
    Q_PROPERTY(QDateTime timestamp MEMBER timestamp)
public:
    QString title;
    int pageId;
    QDateTime timestamp;
};

#endif // WIKIPEDIA_MODELS_H
