#ifndef ARTICLE_PREVIEW_MODEL_H
#define ARTICLE_PREVIEW_MODEL_H

#include "wikipedia_summary_client.h"
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QQmlEngine>
#include <QObject>
#include <functional>

class ArticlePreviewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Status status READ status NOTIFY changed)
    Q_PROPERTY(QString title READ title NOTIFY changed)
    Q_PROPERTY(QString summaryHtml READ summaryHtml NOTIFY changed)
    Q_PROPERTY(QString thumbnailUrl READ thumbnailUrl NOTIFY changed)
    Q_PROPERTY(int pageId READ pageId NOTIFY changed)
    Q_PROPERTY(QString requestedTitle READ requestedTitle NOTIFY changed)
    Q_PROPERTY(bool suppressed READ suppressed NOTIFY changed)
  public:
    using NowFunction = std::function<QDateTime()>;
    static constexpr int CacheLimit = 50;
    static constexpr qint64 CacheTtlSeconds = 300;
    enum Status { Idle, Loading, Ready, Unavailable };
    Q_ENUM(Status)
    explicit ArticlePreviewModel(QObject *parent = nullptr);
    ArticlePreviewModel(NowFunction now, QObject *parent);
    Status status() const { return m_status; }
    QString title() const { return m_summary.value("title").toString(); }
    QString summaryHtml() const { return m_summary.value("html").toString(); }
    QString thumbnailUrl() const { return m_summary.value("thumbnail").toString(); }
    int pageId() const { return m_summary.value("pageId").toInt(); }
    QString requestedTitle() const { return m_requestedTitle; }
    bool suppressed() const { return m_suppressed; }
    /** @brief Request a preview, returning a fresh cached result when available. */
    Q_INVOKABLE void requestPreview(const QString &title);
    /** @brief Cancel the current preview request and clear the displayed item. */
    Q_INVOKABLE void clearPreview();
  signals:
    void changed();
    void summaryReady();
  private:
    struct CacheEntry { QVariantMap summary; QDateTime saved; };
    void setStatus(Status status);
    void cache(const QString &key, const QVariantMap &summary);
    QString key(const QString &title, const QString &language) const;
    WikipediaSummaryClient m_client;
    Status m_status = Idle;
    QString m_requestedTitle;
    QString m_language = QStringLiteral("en");
    QVariantMap m_summary;
    bool m_suppressed = false;
    QHash<QString, CacheEntry> m_cache;
    QList<QString> m_lru;
    NowFunction m_now;
};

#endif
