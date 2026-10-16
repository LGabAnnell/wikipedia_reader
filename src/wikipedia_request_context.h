#ifndef WIKIPEDIA_REQUEST_CONTEXT_H
#define WIKIPEDIA_REQUEST_CONTEXT_H

#include <QNetworkReply>
#include <QSet>
#include <QString>

struct WikipediaRequestContext {
    QString language;
    quint64 generation = 0;
};

/** @brief Captures edition and generation and invalidates outstanding replies. */
class WikipediaRequestTracker : public QObject {
  public:
    WikipediaRequestContext context() const { return {m_language, m_generation}; }
    bool isCurrent(const WikipediaRequestContext &context) const {
        return context.language == m_language && context.generation == m_generation;
    }

    void setLanguage(const QString &language) {
        if (m_language == language)
            return;
        cancelPendingRequests();
        m_language = language;
    }

    void track(QNetworkReply *reply, const WikipediaRequestContext &context) {
        if (!reply)
            return;
        m_replies.insert(reply);
        QObject::connect(reply, &QObject::destroyed, this, [this, reply]() { m_replies.remove(reply); });
        QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, context]() {
            m_replies.remove(reply);
            Q_UNUSED(context);
        });
    }

    void cancelPendingRequests() {
        ++m_generation;
        const auto replies = m_replies;
        for (QNetworkReply *reply : replies) {
            if (reply && !reply->isFinished())
                reply->abort();
        }
        m_replies.clear();
    }

  private:
    QString m_language = QStringLiteral("en");
    quint64 m_generation = 0;
    QSet<QNetworkReply *> m_replies;
};

#endif // WIKIPEDIA_REQUEST_CONTEXT_H
