#include "HistoryDatabase.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>

HistoryDatabase::HistoryDatabase(QObject *parent) : QObject(parent) {}

HistoryDatabase::~HistoryDatabase() {
    if (!QCoreApplication::instanceExists()) {
        return;
    }

    if (QSqlDatabase::contains("history_db")) {
        QSqlDatabase::database("history_db").close();
        QSqlDatabase::removeDatabase("history_db");
    }
}

bool HistoryDatabase::initialize() {
    if (m_initialized)
        return true;
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

    QSqlDatabase db = QSqlDatabase::contains("history_db")
        ? QSqlDatabase::database("history_db", false)
        : QSqlDatabase::addDatabase("QSQLITE", "history_db");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qWarning() << "[HistoryDatabase] Failed to open database:" << db.lastError().text();
        emit databaseError(tr("Failed to open history database: %1").arg(db.lastError().text()));
        return false;
    }

    // Ensure the table and index exist
    if (!migrateSchema()) {
        return false;
    }

    if (!db.transaction()) {
        emit databaseError(tr("Failed to start history retention: %1").arg(db.lastError().text()));
        return false;
    }
    if (!pruneHistory(MAX_HISTORY_ITEMS)) {
        db.rollback();
        return false;
    }
    if (!db.commit()) {
        emit databaseError(tr("Failed to commit history retention: %1").arg(db.lastError().text()));
        db.rollback();
        return false;
    }
    m_initialized = true;
    emit databaseInitialized();
    return true;
}

bool HistoryDatabase::migrateSchema() {
    QSqlDatabase db = QSqlDatabase::database("history_db");
    QSqlQuery query(db);
    const QString versionQuery = "PRAGMA user_version";
    query.prepare(versionQuery);
    if (!executeQuery(query, versionQuery, tr("read history schema version")) || !query.next())
        return false;
    const int version = query.value(0).toInt();
    query.finish();
    if (version == 1)
        return true;
    if (version != 0) {
        emit databaseError(tr("Unsupported history schema version: %1").arg(version));
        return false;
    }
    if (!db.transaction()) {
        emit databaseError(tr("Failed to start history migration: %1").arg(db.lastError().text()));
        return false;
    }

    // All changes, including the version, roll back together on failure.
    const QStringList statements = {
        "CREATE TABLE IF NOT EXISTS history (id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "page_id INTEGER NOT NULL, title TEXT NOT NULL, timestamp DATETIME NOT NULL)",
        "ALTER TABLE history ADD COLUMN language TEXT",
        "DROP INDEX IF EXISTS idx_page_id",
        "CREATE UNIQUE INDEX idx_history_language_page_id ON history(language, page_id)",
        "PRAGMA user_version = 1"
    };
    for (const QString &statement : statements) {
        query.prepare(statement);
        if (!executeQuery(query, statement, tr("migrate history schema"))) {
            db.rollback();
            return false;
        }
        query.finish();
    }
    if (!db.commit()) {
        emit databaseError(tr("Failed to commit history migration: %1").arg(db.lastError().text()));
        db.rollback();
        return false;
    }
    return true;
}

bool HistoryDatabase::ensureReady() {
    if (m_initialized && QSqlDatabase::database("history_db", false).isOpen())
        return true;
    emit databaseError(tr("History database is not initialized"));
    return false;
}

QVector<history_item> HistoryDatabase::loadHistory(int limit) {
    QVector<history_item> history;
    loadHistory(limit, history);
    return history;
}

bool HistoryDatabase::loadHistory(int limit, QVector<history_item> &output) {
    QVector<history_item> history;
    QSqlDatabase db = QSqlDatabase::database("history_db");
    if (!ensureReady()) {
        return false;
    }

    QSqlQuery query(db);
    QString selectQuery = "SELECT page_id, title, timestamp, language FROM history ORDER BY timestamp DESC, id DESC LIMIT :limit";

    query.prepare(selectQuery);
    query.bindValue(":limit", limit);
    if (!executeQuery(query, selectQuery, "load history")) {
        return false;
    }

    while (query.next()) {
        history_item item;
        item.pageId = query.value("page_id").toInt();
        item.title = query.value("title").toString();
        item.language = query.value("language").toString();
        item.timestamp = query.value("timestamp").toDateTime();
        history.append(item);
    }

    if (query.lastError().isValid()) {
        emit databaseError(tr("Failed to load history: %1. Query: %2").arg(query.lastError().text(), selectQuery));
        return false;
    }
    output = history;
    return true;
}

bool HistoryDatabase::addToHistory(const QString &title, int pageId, const QString &language, const QDateTime &timestamp,
                                   QVector<history_item> *snapshot) {
    QSqlDatabase db = QSqlDatabase::database("history_db");
    if (!ensureReady()) {
        return false;
    }

    if (language.trimmed().isEmpty()) {
        emit databaseError(tr("History language must not be blank"));
        return false;
    }

    // Start transaction for better performance
    if (!db.transaction()) {
        qWarning() << "[HistoryDatabase] Failed to start database transaction:" << db.lastError().text();
        emit databaseError(tr("Failed to start database transaction: %1").arg(db.lastError().text()));
        return false;
    }

    QSqlQuery query(db);

    // First remove any existing entry for this edition and page ID
    QString deleteQuery = "DELETE FROM history WHERE language = :language AND page_id = :page_id";
    if (!query.prepare(deleteQuery)) {
        qWarning() << "[HistoryDatabase] Failed to prepare delete query:" << query.lastError().text();
        emit databaseError(tr("Failed to prepare delete query: %1. Query: %2").arg(query.lastError().text(), deleteQuery));
        db.rollback();
        return false;
    }

    query.bindValue(":page_id", pageId);
    query.bindValue(":language", language);
    if (!executeQuery(query, deleteQuery, "delete existing history item")) {
        db.rollback();
        return false;
    }

    // Insert the new item
    QString insertQuery = "INSERT INTO history (page_id, title, timestamp, language) VALUES (:page_id, :title, :timestamp, :language)";
    if (!query.prepare(insertQuery)) {
        qWarning() << "[HistoryDatabase] Failed to prepare insert query:" << query.lastError().text();
        emit databaseError(tr("Failed to prepare insert query: %1. Query: %2").arg(query.lastError().text(), insertQuery));
        db.rollback();
        return false;
    }

    query.bindValue(":page_id", pageId);
    query.bindValue(":language", language);
    query.bindValue(":title", title);
    query.bindValue(":timestamp", timestamp);
    if (!executeQuery(query, insertQuery, "insert new history item")) {
        db.rollback();
        return false;
    }

    QVector<history_item> retained;
    if (!pruneHistory(MAX_HISTORY_ITEMS) || !loadHistory(MAX_HISTORY_ITEMS, retained)) {
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

    if (snapshot)
        *snapshot = retained;
    return true;
}

bool HistoryDatabase::clearHistory() {
    QSqlDatabase db = QSqlDatabase::database("history_db");
    if (!ensureReady()) {
        return false;
    }

    if (!db.transaction()) {
        emit databaseError(tr("Failed to start history clearing: %1").arg(db.lastError().text()));
        return false;
    }
    QSqlQuery query(db);
    QString clearQuery = "DELETE FROM history";
    query.prepare(clearQuery);
    if (!executeQuery(query, clearQuery, tr("clear history"))) {
        db.rollback();
        return false;
    }
    if (!db.commit()) {
        emit databaseError(tr("Failed to commit history clearing: %1").arg(db.lastError().text()));
        db.rollback();
        return false;
    }
    return true;
}

bool HistoryDatabase::removeOldestItems(int limit) {
    QSqlDatabase db = QSqlDatabase::database("history_db");
    if (!ensureReady()) {
        return false;
    }

    return pruneHistory(limit);
}

bool HistoryDatabase::pruneHistory(int limit) {
    QSqlQuery query(QSqlDatabase::database("history_db"));
    QString deleteOldestQuery = R"##(DELETE FROM history WHERE id NOT IN (
        SELECT id FROM history ORDER BY timestamp DESC, id DESC LIMIT :limit
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