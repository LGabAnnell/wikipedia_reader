#include "wikipedia_summary_client.h"
#include "wikipedia_network_access_manager.h"
#include "html_processor.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

WikipediaSummaryClient::WikipediaSummaryClient(QObject *parent)
    : QObject(parent), m_network(WikipediaNetwork::createNetworkAccessManager(this)) {}
WikipediaSummaryClient::~WikipediaSummaryClient() { cancelPendingRequests(); }

void WikipediaSummaryClient::setLanguage(const QString &language) { m_requests.setLanguage(language); }
void WikipediaSummaryClient::cancelPendingRequests() { m_requests.cancelPendingRequests(); }

void WikipediaSummaryClient::fetchSummary(const QString &title) {
    const auto context = m_requests.context();
    const QString encoded = QString::fromLatin1(QUrl::toPercentEncoding(title));
    const QUrl url(QStringLiteral("https://%1.wikipedia.org/api/rest_v1/page/summary/%2")
                       .arg(context.language, encoded));
    QNetworkRequest request(url);
    auto summaryContext = context;
    summaryContext.deadlineMs = wikipediaMonotonicNowMs() + RequestTimeoutMs;
    applyRequestDeadline(request, summaryContext);
    QNetworkReply *reply = m_network->get(request);
    m_requests.track(reply, context);
    auto *timeout = new QTimer(reply);
    timeout->setSingleShot(true);
    timeout->start(RequestTimeoutMs);
    connect(timeout, &QTimer::timeout, reply, [reply]() {
        if (!reply->isFinished())
            reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, context, title]() {
        if (!m_requests.isCurrent(context)) {
            reply->deleteLater();
            return;
        }
        const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError || (status && (status < 200 || status >= 300))) {
            emit errorOccurred(title, tr("Unable to load article preview"));
            reply->deleteLater();
            return;
        }
        const QVariantMap summary = parseSummary(reply->readAll());
        if (summary.isEmpty())
            emit errorOccurred(title, tr("Article preview is unavailable"));
        else
            emit summaryReceived(title, summary);
        reply->deleteLater();
    });
}

QVariantMap WikipediaSummaryClient::parseSummary(const QByteArray &data) {
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return {};
    const auto root = document.object();
    const QString title = root.value(QStringLiteral("title")).toString();
    const QString extract = root.value(QStringLiteral("extract")).toString().trimmed();
    const int pageId = root.value(QStringLiteral("pageid")).toInt();
    const int ns = root.value(QStringLiteral("namespace")).toObject()
                       .value(QStringLiteral("id")).toInt(root.value(QStringLiteral("ns")).toInt());
    if (title.isEmpty() || pageId <= 0 || (ns == 0 && extract.isEmpty()))
        return {};
    const QString html = root.value(QStringLiteral("extract_html")).toString();
    return {{QStringLiteral("title"), title}, {QStringLiteral("extract"), extract},
            {QStringLiteral("html"), html}, {QStringLiteral("pageId"), pageId},
            {QStringLiteral("namespace"), ns},
            {QStringLiteral("thumbnail"), root.value(QStringLiteral("thumbnail")).toObject()
                                                    .value(QStringLiteral("source")).toString()}};
}
