// src/HistoryState.cpp
#include "HistoryState.h"

HistoryState::HistoryState(QObject *parent) : QObject(parent) {
}

QVector<history_item> HistoryState::history() const {
    return m_history;
}

void HistoryState::addToHistory(const QString &title, int pageId) {
    if (itemExistsInHistory(pageId)) {
        for (int i = 0; i < m_history.size(); ++i) {
            if (m_history[i].pageId == pageId) {
                m_history.remove(i);
                break;
            }
        }
    }

    history_item newItem;
    newItem.title = title;
    newItem.pageId = pageId;
    newItem.timestamp = QDateTime::currentDateTime();
    m_history.prepend(newItem);

    if (m_history.size() > MAX_HISTORY_ITEMS) {
        m_history.removeLast();
    }

    emit historyChanged();
}

void HistoryState::clearHistory() {
    m_history.clear();
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