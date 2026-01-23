#include "HistoryDatabase.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

HistoryDatabase::HistoryDatabase(QObject *parent) : QObject(parent) {
    // Initialize database in the constructor
    initialize();
}

HistoryDatabase::~HistoryDatabase() {
    if (QSqlDatabase::contains("history_db")) {
        QSqlDatabase::database("history_db").close();
        QSqlDatabase::removeDatabase("history_db");
    }
}

bool HistoryDatabase::initialize() {
    // Get the application's data location
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    // Create the directory if it doesn't exist
    if (!QDir().mkpath(dataPath)) {
        qWarning() << "[HistoryDatabase] Failed to create directory:" << dataPath;
        emit databaseError(tr("Failed to create application data directory"));
        return false;
    }

    // Set up the database connection
    QString dbPath = dataPath + "/history.db";

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "history_db");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qWarning() << "[HistoryDatabase] Failed to open database:" << db.lastError().text();
        emit databaseError(tr("Failed to open history database: %1").arg(db.lastError().text()));
        return false;
    }

    // Ensure the table and index exist
    if (!ensureTableExists() || !ensureIndexExists()) {
        return false;
    }

    emit databaseInitialized();
    return true;
}

bool HistoryDatabase::ensureTableExists() {
    QSqlDatabase db = QSqlDatabase::database("history_db");
    if (!db.isOpen()) {
        qWarning() << "[HistoryDatabase] Database is not open";
        emit databaseError(tr("Database is not open"));
        return false;
    }

    QSqlQuery query(db);
    QString createTableQuery = R"###(
        CREATE TABLE IF NOT EXISTS history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            page_id INTEGER NOT NULL,
            title TEXT NOT NULL,
            timestamp DATETIME NOT NULL
        )
    )###";

    query.prepare(createTableQuery);
    return executeQuery(query, createTableQuery, "create history table");
}

bool HistoryDatabase::ensureIndexExists() {
    QSqlDatabase db = QSqlDatabase::database("history_db");
    if (!db.isOpen()) {
        emit databaseError(tr("Database is not open"));
        return false;
    }

    QSqlQuery query(db);
    QString createIndexQuery = "CREATE INDEX IF NOT EXISTS idx_page_id ON history(page_id)";
    query.prepare(createIndexQuery);
    return executeQuery(query, createIndexQuery, "create history index");
}

QVector<history_item> HistoryDatabase::loadHistory(int limit) {
    QVector<history_item> history;
    QSqlDatabase db = QSqlDatabase::database("history_db");
    if (!db.isOpen()) {
        qWarning() << "[HistoryDatabase] Database is not open";
        emit databaseError(tr("Database is not open"));
        return history;
    }

    QSqlQuery query(db);
    QString selectQuery = "SELECT page_id, title, timestamp FROM history ORDER BY timestamp DESC LIMIT :limit";

    query.prepare(selectQuery);
    query.bindValue(":limit", limit);
    if (!executeQuery(query, selectQuery, "load history")) {
        return history;
    }

    while (query.next()) {
        history_item item;
        item.pageId = query.value("page_id").toInt();
        item.title = query.value("title").toString();
        item.timestamp = query.value("timestamp").toDateTime();
        history.append(item);
    }

    return history;
}

bool HistoryDatabase::addToHistory(const QString &title, int pageId, const QDateTime &timestamp) {
    QSqlDatabase db = QSqlDatabase::database("history_db");
    if (!db.isOpen()) {
        qWarning() << "[HistoryDatabase] Database is not open";
        emit databaseError(tr("Database is not open"));
        return false;
    }

    // Start transaction for better performance
    if (!db.transaction()) {
        qWarning() << "[HistoryDatabase] Failed to start database transaction:" << db.lastError().text();
        emit databaseError(tr("Failed to start database transaction: %1").arg(db.lastError().text()));
        return false;
    }

    QSqlQuery query(db);

    // First remove any existing entry with the same pageId
    QString deleteQuery = "DELETE FROM history WHERE page_id = :page_id";
    if (!query.prepare(deleteQuery)) {
        qWarning() << "[HistoryDatabase] Failed to prepare delete query:" << query.lastError().text();
        emit databaseError(tr("Failed to prepare delete query: %1").arg(query.lastError().text()));
        db.rollback();
        return false;
    }

    query.bindValue(":page_id", pageId);
    if (!executeQuery(query, deleteQuery, "delete existing history item")) {
        db.rollback();
        return false;
    }

    // Insert the new item
    QString insertQuery = "INSERT INTO history (page_id, title, timestamp) VALUES (:page_id, :title, :timestamp)";
    if (!query.prepare(insertQuery)) {
        qWarning() << "[HistoryDatabase] Failed to prepare insert query:" << query.lastError().text();
        emit databaseError(tr("Failed to prepare insert query: %1").arg(query.lastError().text()));
        db.rollback();
        return false;
    }

    query.bindValue(":page_id", pageId);
    query.bindValue(":title", title);
    query.bindValue(":timestamp", timestamp);
    if (!executeQuery(query, insertQuery, "insert new history item")) {
        db.rollback();
        return false;
    }

    // Commit the transaction
    if (!db.commit()) {
        qWarning() << "[HistoryDatabase] Failed to commit database transaction:" << db.lastError().text();
        emit databaseError(tr("Failed to commit database transaction: %1").arg(db.lastError().text()));
        db.rollback();
        return false;
    }

    return true;
}

bool HistoryDatabase::clearHistory() {
    QSqlDatabase db = QSqlDatabase::database("history_db");
    if (!db.isOpen()) {
        qWarning() << "[HistoryDatabase] Database is not open";
        emit databaseError(tr("Database is not open"));
        return false;
    }

    QSqlQuery query(db);
    QString clearQuery = "DELETE FROM history";
    return executeQuery(query, clearQuery, "clear history");
}

bool HistoryDatabase::removeOldestItems(int limit) {
    QSqlDatabase db = QSqlDatabase::database("history_db");
    if (!db.isOpen()) {
        qWarning() << "[HistoryDatabase] Database is not open";
        emit databaseError(tr("Database is not open"));
        return false;
    }

    QSqlQuery query(db);
    QString deleteOldestQuery = R"##(DELETE FROM history WHERE id NOT IN (
        SELECT id FROM history ORDER BY timestamp DESC LIMIT :limit
    ))##";

    query.prepare(deleteOldestQuery);
    query.bindValue(":limit", limit);
    return executeQuery(query, deleteOldestQuery, "delete oldest history item");
}

bool HistoryDatabase::executeQuery(QSqlQuery &query, const QString &queryText, const QString &operationDescription) {
    if (!query.exec()) {
        qWarning() << "[HistoryDatabase] Failed to" << operationDescription << ":" << query.lastError().text();
        emit databaseError(tr("Failed to %1: %2. Query: %3")
                         .arg(operationDescription)
                         .arg(query.lastError().text())
                         .arg(queryText));
        return false;
    }
    return true;
}