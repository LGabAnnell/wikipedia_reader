// src/modules/HistoryModule/HistoryModel.cpp
#include "HistoryModel.h"

HistoryModel::HistoryModel(QObject *parent) : QObject(parent) {
    // Initialize empty history
}

QVector<history_item> HistoryModel::history() const { return m_history; }

void HistoryModel::addToHistory(const QString &title, int pageId, const QString &language) {
    if (language.trimmed().isEmpty())
        return;

    // Check if item already exists in history
    if (itemExistsInHistory(pageId, language)) {
        // Remove existing item to update its position (move to top)
        for (int i = 0; i < m_history.size(); ++i) {
            if (m_history[i].pageId == pageId && m_history[i].language == language) {
                m_history.remove(i);
                break;
            }
        }
    }

    // Add new item to the beginning of history
    history_item newItem;
    newItem.title = title;
    newItem.language = language;
    newItem.pageId = pageId;
    newItem.timestamp = QDateTime::currentDateTime();
    m_history.prepend(newItem);

    // Limit history size
    if (m_history.size() > MAX_HISTORY_ITEMS) {
        m_history.removeLast();
    }

    emit historyChanged();
}

void HistoryModel::clearHistory() {
    m_history.clear();
    emit historyChanged();
}

void HistoryModel::removeFromHistory(int index) {
    if (index >= 0 && index < m_history.size()) {
        m_history.remove(index);
        emit historyChanged();
    }
}

bool HistoryModel::itemExistsInHistory(int pageId, const QString &language) {
    for (const auto &item : std::as_const(m_history)) {
        if (item.pageId == pageId && item.language == language) {
            return true;
        }
    }
    return false;
}
