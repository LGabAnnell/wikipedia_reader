#ifndef HISTORYDATABASE_H
#define HISTORYDATABASE_H

#include "wikipedia_models.h" // For history_item
#include <QDateTime>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QVector>

class HistoryDatabase : public QObject {
    Q_OBJECT

  public:
    explicit HistoryDatabase(QObject *parent = nullptr);
    ~HistoryDatabase();

    // Database operations
    bool initialize();
    QVector<history_item> loadHistory(int limit);
    /** @brief Persist a visit identified by a nonblank Wikipedia language and page ID. */
    bool addToHistory(const QString &title, int pageId, const QString &language, const QDateTime &timestamp);
    bool clearHistory();
    bool removeOldestItems(int limit);

  signals:
    void databaseError(const QString &errorMessage);
    void databaseInitialized();

  private:
    bool executeQuery(QSqlQuery &query, const QString &queryText, const QString &operationDescription);
    bool migrateSchema();
    bool ensureReady();
    bool m_initialized = false;
};

#endif // HISTORYDATABASE_H