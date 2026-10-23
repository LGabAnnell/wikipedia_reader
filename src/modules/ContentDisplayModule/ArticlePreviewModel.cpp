#include "ArticlePreviewModel.h"
#include "SettingsState.h"
#include "html_processor.h"
#include <QTextDocumentFragment>
#include <utility>

ArticlePreviewModel::ArticlePreviewModel(QObject *parent)
    : ArticlePreviewModel([]() { return QDateTime::currentDateTimeUtc(); }, parent) {}

ArticlePreviewModel::ArticlePreviewModel(NowFunction now, QObject *parent) : QObject(parent), m_now(std::move(now)) {
    auto settings = SettingsState::instance();
    if (settings) {
        m_language = settings->language();
        m_client.setLanguage(m_language);
        connect(settings, &SettingsState::languageTransitionStarted, this, [this]() {
            m_client.cancelPendingRequests();
            m_cache.clear();
            m_lru.clear();
            m_requestedTitle.clear();
            m_summary.clear();
            m_suppressed = false;
            setStatus(Idle);
            emit changed();
        });
        connect(settings, &SettingsState::languageChanged, this, [this, settings]() {
            m_language = settings->language();
            m_client.setLanguage(m_language);
        });
    }
    connect(&m_client, &WikipediaSummaryClient::summaryReceived, this,
            [this](const QString &target, const QVariantMap &summary) {
                if (target != m_requestedTitle || m_status != Loading)
                    return;
                if (summary.value("namespace").toInt() != 0) {
                    cache(key(target, m_language), {{"namespace", summary.value("namespace")}});
                    m_summary.clear();
                    m_suppressed = true;
                    setStatus(Unavailable);
                    emit changed();
                    return;
                }
                QVariantMap result = summary;
                QString html = result.value("html").toString();
                if (html.isEmpty())
                    html = QStringLiteral("<p>%1</p>").arg(result.value("extract").toString().toHtmlEscaped());
                result.insert("html", HtmlProcessor::sanitizeSummaryHtml(html));
                if (QTextDocumentFragment::fromHtml(result.value("html").toString())
                        .toPlainText().trimmed().isEmpty())
                    result.insert("html", QStringLiteral("<p>%1</p>").arg(
                        result.value("extract").toString().toHtmlEscaped()));
                m_summary = result;
                cache(key(target, m_language), result);
                setStatus(Ready);
                emit changed();
                emit summaryReady();
            });
    connect(&m_client, &WikipediaSummaryClient::errorOccurred, this,
            [this](const QString &target, const QString &) {
                if (target != m_requestedTitle || m_status != Loading)
                    return;
                m_summary.clear();
                setStatus(Unavailable);
                emit changed();
            });
}

QString ArticlePreviewModel::key(const QString &title, const QString &language) const {
    QString normalized = title;
    normalized.replace(QLatin1Char('_'), QLatin1Char(' '));
    return language + QLatin1Char(':') + normalized;
}
void ArticlePreviewModel::setStatus(Status status) {
    if (m_status == status)
        return;
    m_status = status;
    emit changed();
}
void ArticlePreviewModel::cache(const QString &cacheKey, const QVariantMap &summary) {
    m_cache.insert(cacheKey, {summary, m_now()});
    m_lru.removeAll(cacheKey);
    m_lru.append(cacheKey);
    while (m_lru.size() > CacheLimit)
        m_cache.remove(m_lru.takeFirst());
}
void ArticlePreviewModel::requestPreview(const QString &title) {
    if (title.trimmed().isEmpty()) {
        clearPreview();
        return;
    }
    if (title == m_requestedTitle && (m_status == Loading || m_status == Ready))
        return;
    m_client.cancelPendingRequests();
    m_requestedTitle = title;
    m_summary.clear();
    m_suppressed = false;
    const QString cacheKey = key(title, m_language);
    auto it = m_cache.find(cacheKey);
    if (it != m_cache.end() && it->saved.secsTo(m_now()) < CacheTtlSeconds) {
        m_suppressed = it->summary.value("namespace").toInt() != 0;
        m_summary = m_suppressed ? QVariantMap{} : it->summary;
        m_lru.removeAll(cacheKey);
        m_lru.append(cacheKey);
        setStatus(m_suppressed ? Unavailable : Ready);
        emit changed();
        if (!m_suppressed)
            emit summaryReady();
        return;
    }
    if (it != m_cache.end()) {
        m_cache.erase(it);
        m_lru.removeAll(cacheKey);
    }
    setStatus(Loading);
    emit changed();
    m_client.fetchSummary(title);
}
void ArticlePreviewModel::clearPreview() {
    m_client.cancelPendingRequests();
    m_requestedTitle.clear();
    m_summary.clear();
    m_suppressed = false;
    setStatus(Idle);
    emit changed();
}
