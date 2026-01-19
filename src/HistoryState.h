// src/HistoryState.h
#ifndef HISTORYSTATE_H
#define HISTORYSTATE_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QDateTime>
#include <QQmlEngine>
#include "wikipedia_client.h"

class HistoryState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Singleton")

    Q_PROPERTY(QVector<history_item> history READ history NOTIFY historyChanged)

public:
    explicit HistoryState(QObject *parent = nullptr);

    QVector<history_item> history() const;

public slots:
    void addToHistory(const QString &title, int pageId);
    void clearHistory();

signals:
    void historyChanged();

private:
    QVector<history_item> m_history;
    static const int MAX_HISTORY_ITEMS = 50;

    bool itemExistsInHistory(int pageId);
};

#endif // HISTORYSTATE_H