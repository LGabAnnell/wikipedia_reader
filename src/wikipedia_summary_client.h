#ifndef WIKIPEDIA_SUMMARY_CLIENT_H
#define WIKIPEDIA_SUMMARY_CLIENT_H

#include "wikipedia_request_context.h"
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QVariantMap>

class WikipediaSummaryClient final : public QObject {
    Q_OBJECT
  public:
    static constexpr int RequestTimeoutMs = 10000;
    explicit WikipediaSummaryClient(QObject *parent = nullptr);
    ~WikipediaSummaryClient() override;
    /** @brief Select the Wikipedia edition used for future summaries. */
    void setLanguage(const QString &language);
    /** @brief Cancel pending summary requests and reject their completions. */
    void cancelPendingRequests();
    /** @brief Fetch one article summary asynchronously. */
    void fetchSummary(const QString &title);
    /** @brief Parse a Page Content Service summary response. */
    static QVariantMap parseSummary(const QByteArray &data);
  signals:
    void summaryReceived(const QString &requestedTitle, const QVariantMap &summary);
    void errorOccurred(const QString &requestedTitle, const QString &message);
  private:
    QNetworkAccessManager *m_network = nullptr;
    WikipediaRequestTracker m_requests;
};

#endif
