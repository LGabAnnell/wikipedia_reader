#include "HistoryState.h"
#include "HistoryDatabase.h"

HistoryState::HistoryState(QObject *parent) : QObject(parent) {
    m_database = new HistoryDatabase(this);
    connect(m_database, &HistoryDatabase::databaseError, this, [this](const QString &message) {
        setDatabaseError(message);
        emit databaseError(message);
    });
    if (m_database->initialize() && m_database->loadHistory(HistoryDatabase::MAX_HISTORY_ITEMS, m_history)) {
        m_databaseReady = true;
        setDatabaseError({});
        emit databaseReadyChanged();
        emit databaseInitialized();
    }
}

HistoryState::~HistoryState() = default;
QVector<history_item> HistoryState::history() const { return m_history; }

void HistoryState::setDatabaseError(const QString &message) {
    if (m_lastDatabaseError == message)
        return;
    m_lastDatabaseError = message;
    emit lastDatabaseErrorChanged();
}

void HistoryState::addToHistory(const QString &title, int pageId, const QString &language) {
    if (language.trimmed().isEmpty() || !m_databaseReady)
        return;
    QVector<history_item> retained;
    if (!m_database->addToHistory(title, pageId, language, QDateTime::currentDateTime(), &retained))
        return;
    m_history = retained;
    setDatabaseError({});
    emit historyChanged();
}

void HistoryState::clearHistory() {
    if (!m_databaseReady || !m_database->clearHistory())
        return;
    m_history.clear();
    setDatabaseError({});
    emit historyChanged();
}
