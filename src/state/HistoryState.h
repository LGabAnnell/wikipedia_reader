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

    Q_PROPERTY(bool databaseReady READ databaseReady NOTIFY databaseReadyChanged)
    Q_PROPERTY(QString lastDatabaseError READ lastDatabaseError NOTIFY lastDatabaseErrorChanged)
    Q_PROPERTY(QVector<history_item> history READ history NOTIFY historyChanged)

  public:
    explicit HistoryState(QObject *parent = nullptr);
    ~HistoryState();

    QVector<history_item> history() const;
    bool databaseReady() const { return m_databaseReady; }
    QString lastDatabaseError() const { return m_lastDatabaseError; }

  public slots:
    /** @brief Record a visit identified by a nonblank Wikipedia language and page ID. */
    void addToHistory(const QString &title, int pageId, const QString &language);
    void clearHistory();

  signals:
    void historyChanged();
    void databaseReadyChanged();
    void lastDatabaseErrorChanged();
    void databaseError(const QString &errorMessage);
    void databaseInitialized();

  private:
    QVector<history_item> m_history;
    bool m_databaseReady = false;
    QString m_lastDatabaseError;
    void setDatabaseError(const QString &message);
    HistoryDatabase *m_database;

};

#endif // HISTORYSTATE_H