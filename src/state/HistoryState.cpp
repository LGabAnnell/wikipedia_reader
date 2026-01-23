#include "HistoryState.h"
#include "HistoryDatabase.h"
#include <QDebug>
HistoryState::HistoryState(QObject *parent) : QObject(parent) {
    m_database = new HistoryDatabase(parent);

    // Connect signals
    connect(m_database, &HistoryDatabase::databaseError, this, &HistoryState::databaseError);
    connect(m_database, &HistoryDatabase::databaseInitialized, this, &HistoryState::databaseInitialized);

    // Load history from database
    m_history = m_database->loadHistory(MAX_HISTORY_ITEMS);
}
HistoryState::~HistoryState() {
    // m_database is deleted automatically by Qt's parent-child mechanism
}
QVector<history_item> HistoryState::history() const {
    return m_history;
}

void HistoryState::addToHistory(const QString &title, int pageId) {
    // First check if item exists in memory
    if (itemExistsInHistory(pageId)) {
        for (int i = 0; i < m_history.size(); ++i) {
            if (m_history[i].pageId == pageId) {
                m_history.remove(i);
                break;
            }
        }
    }

    // Create new item
    history_item newItem;
    newItem.title = title;
    newItem.pageId = pageId;
    newItem.timestamp = QDateTime::currentDateTime();
    m_history.prepend(newItem);

    // Remove oldest item if we exceed the limit
    if (m_history.size() > MAX_HISTORY_ITEMS) {
        m_history.removeLast();
    }
    
    // Update database
    if (!m_database->addToHistory(title, pageId, newItem.timestamp)) {
        qWarning() << "[HistoryState] Failed to add item to history database.";
    }

    // Remove oldest items from database if we exceed the limit
    if (m_history.size() > MAX_HISTORY_ITEMS) {
        if (!m_database->removeOldestItems(MAX_HISTORY_ITEMS)) {
            qWarning() << "[HistoryState] Failed to remove oldest items from history database.";
        }
    }
            emit historyChanged();
        }

void HistoryState::clearHistory() {
    m_history.clear();
    if (!m_database->clearHistory()) {
        qWarning() << "[HistoryState] Failed to clear history database.";
    }
        emit historyChanged();
    }

bool HistoryState::itemExistsInHistory(int pageId) {
    for (const auto &item : m_history) {
        if (item.pageId == pageId) {
            return true;
        }
    }
    return false;
}

