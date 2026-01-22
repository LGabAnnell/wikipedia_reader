#ifndef HISTORYDATABASE_H
#define HISTORYDATABASE_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include "wikipedia_client.h" // For history_item

class HistoryDatabase : public QObject {
    Q_OBJECT

public:
    explicit HistoryDatabase(QObject *parent = nullptr);
    ~HistoryDatabase();

    // Database operations
    bool initialize();
    QVector<history_item> loadHistory(int limit);
    bool addToHistory(const QString &title, int pageId, const QDateTime &timestamp);
    bool clearHistory();
    bool removeOldestItems(int limit);

signals:
    void databaseError(const QString &errorMessage);
    void databaseInitialized();

private:
    bool executeQuery(QSqlQuery &query, const QString &queryText, const QString &operationDescription);
    bool ensureTableExists();
    bool ensureIndexExists();
};

#endif // HISTORYDATABASE_H