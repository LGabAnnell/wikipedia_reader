// src/modules/HistoryModule/HistoryModel.h
#ifndef HISTORYMODEL_H
#define HISTORYMODEL_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QDateTime>
#include <QQmlEngine>

// History item structure
struct history_item {
    QString title;
    int pageId;
    QDateTime timestamp;
};

class HistoryModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVector<history_item> history READ history NOTIFY historyChanged)

public:
    explicit HistoryModel(QObject *parent = nullptr);

    // History property accessor
    QVector<history_item> history() const;

public slots:
    void addToHistory(const QString &title, int pageId);
    void clearHistory();
    void removeFromHistory(int index);

signals:
    void historyChanged();

private:
    QVector<history_item> m_history;
    static const int MAX_HISTORY_ITEMS = 50;

    // Helper method to check if item already exists in history
    bool itemExistsInHistory(int pageId);
};

#endif // HISTORYMODEL_H