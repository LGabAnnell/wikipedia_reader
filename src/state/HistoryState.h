#ifndef HISTORYSTATE_H
#define HISTORYSTATE_H

#include "wikipedia_models.h" // For history_item
#include <QDateTime>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QVector>

class HistoryDatabase; // Forward declaration

class HistoryState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Singleton")

    Q_PROPERTY(QVector<history_item> history READ history NOTIFY historyChanged)

  public:
    explicit HistoryState(QObject *parent = nullptr);
    ~HistoryState();

    QVector<history_item> history() const;

  public slots:
    void addToHistory(const QString &title, int pageId);
    void clearHistory();

  signals:
    void historyChanged();
    void databaseError(const QString &errorMessage);
    void databaseInitialized();

  private:
    QVector<history_item> m_history;
    static const int MAX_HISTORY_ITEMS = 50;
    HistoryDatabase *m_database;

    bool itemExistsInHistory(int pageId);
};

#endif // HISTORYSTATE_H