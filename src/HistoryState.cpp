#include "HistoryState.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QTimer>

HistoryState::HistoryState(QObject *parent) : QObject(parent) {
    // Initialize database first
    initializeDatabase();

    // Then load history from it
    loadHistoryFromDatabase();
}

HistoryState::~HistoryState() {
    if (QSqlDatabase::contains("history_db")) {
        QSqlDatabase::database("history_db").close();
        QSqlDatabase::removeDatabase("history_db");
    }
}

QVector<history_item> HistoryState::history() const {
    return m_history;
}

void HistoryState::initializeDatabase() {
    // Get the application's data location
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    // Create the directory if it doesn't exist
    if (!QDir().mkpath(dataPath)) {
        qWarning() << "[HistoryState] Failed to create directory:" << dataPath;
        emit databaseError(tr("Failed to create application data directory"));
        return;
    }

    // Set up the database connection
    QString dbPath = dataPath + "/history.db";

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "history_db");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qWarning() << "[HistoryState] Failed to open database:" << db.lastError().text();
        emit databaseError(tr("Failed to open history database: %1").arg(db.lastError().text()));
        return;
    }

    // Create the history table if it doesn't exist
    QSqlQuery query(db);
    QString createTableQuery = R"###(
        CREATE TABLE IF NOT EXISTS history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            page_id INTEGER NOT NULL,
            title TEXT NOT NULL,
            timestamp DATETIME NOT NULL
        )
)###";

    query.prepare(createTableQuery); // Prepare the query to avoid SQL injection and ensure proper paramet
    if (!executeQuery(query, createTableQuery, "create history table")) {
        qWarning() << "[HistoryState] Failed to create history table.";
        return;
    }

    // Create the index if it doesn't exist
    QString createIndexQuery = "CREATE INDEX IF NOT EXISTS idx_page_id ON history(page_id)";
    query.prepare(createIndexQuery);
    if (!executeQuery(query, createIndexQuery, "create history index")) {
        qWarning() << "[HistoryState] Failed to create history index.";
        return;
    }

    // Verify the table exists
    if (!query.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='history'")) {
        qWarning() << "[HistoryState] Failed to check if history table exists:" << query.lastError().text();
        return;
    }

    emit databaseInitialized();
}

void HistoryState::loadHistoryFromDatabase() {
    QSqlDatabase db = QSqlDatabase::database("history_db");
    if (!db.isOpen()) {
        qWarning() << "[HistoryState] Database is not open";
        emit databaseError(tr("Database is not open"));
        return;
    }

    // First ensure the table exists by executing the create table query
    QSqlQuery query(db);

    QString selectQuery = "SELECT page_id, title, timestamp FROM history ORDER BY timestamp DESC LIMIT :limit";

    query.prepare(selectQuery);
    query.bindValue(":limit", MAX_HISTORY_ITEMS);
    if (!executeQuery(query, selectQuery, "load history")) {
        qWarning() << "[HistoryState] Failed to execute history load query.";
        return;
    }

    m_history.clear();
    while (query.next()) {
        history_item item;
        item.pageId = query.value("page_id").toInt();
        item.title = query.value("title").toString();
        item.timestamp = query.value("timestamp").toDateTime();
        m_history.append(item);
    }

    emit historyChanged();
}

void HistoryState::addToHistory(const QString &title, int pageId) {
    // First check if item exists in memory
    if (itemExistsInHistory(pageId)) {
        for (int i = 0; i < m_history.size(); ++i) {
            if (m_history[i].pageId == pageId) {
                m_history.remove(i);
                break;
            }
        }
    }

    // Create new item
    history_item newItem;
    newItem.title = title;
    newItem.pageId = pageId;
    newItem.timestamp = QDateTime::currentDateTime();
    m_history.prepend(newItem);

    // Remove oldest item if we exceed the limit
    if (m_history.size() > MAX_HISTORY_ITEMS) {
        m_history.removeLast();
    }

    // Update database
    QSqlDatabase db = QSqlDatabase::database("history_db");
    if (!db.isOpen()) {
        qWarning() << "[HistoryState] Database is not open";
        emit databaseError(tr("Database is not open"));
        emit historyChanged();
        return;
    }

    // Start transaction for better performance
    if (!db.transaction()) {
        qWarning() << "[HistoryState] Failed to start database transaction:" << db.lastError().text();
        emit databaseError(tr("Failed to start database transaction: %1").arg(db.lastError().text()));
        emit historyChanged();
        return;
    }

    QSqlQuery query(db);

    // First remove any existing entry with the same pageId
    QString deleteQuery = "DELETE FROM history WHERE page_id = :page_id";
    if (!query.prepare(deleteQuery)) {
        qWarning() << "[HistoryState] Failed to prepare delete query:" << query.lastError().text();
        emit databaseError(tr("Failed to prepare delete query: %1").arg(query.lastError().text()));
        db.rollback();
        emit historyChanged();
        return;
    }

    query.bindValue(":page_id", pageId);
    if (!executeQuery(query, deleteQuery, "delete existing history item")) {
        qWarning() << "[HistoryState] Failed to execute delete query.";
        db.rollback();
        emit historyChanged();
        return;
    }

    // Insert the new item
    QString insertQuery = "INSERT INTO history (page_id, title, timestamp) VALUES (:page_id, :title, :timestamp)";
    if (!query.prepare(insertQuery)) {
        qWarning() << "[HistoryState] Failed to prepare insert query:" << query.lastError().text();
        emit databaseError(tr("Failed to prepare insert query: %1").arg(query.lastError().text()));
        db.rollback();
        emit historyChanged();
        return;
    }

    query.bindValue(":page_id", pageId);
    query.bindValue(":title", title);
    query.bindValue(":timestamp", newItem.timestamp);
    if (!executeQuery(query, insertQuery, "insert new history item")) {
        qWarning() << "[HistoryState] Failed to execute insert query.";
        db.rollback();
        emit historyChanged();
        return;
    }

    // Remove oldest item if we exceed the limit
    if (m_history.size() > MAX_HISTORY_ITEMS) {
        qDebug() << "[HistoryState] History size exceeds limit of" << MAX_HISTORY_ITEMS << ". Preparing to remove oldest item from database.";
        QString deleteOldestQuery = "DELETE FROM history WHERE id NOT IN (\
            SELECT id FROM history ORDER BY timestamp DESC LIMIT :limit\
        )";

        if (!query.prepare(deleteOldestQuery)) {
            qWarning() << "[HistoryState] Failed to prepare delete oldest query:" << query.lastError().text();
            emit databaseError(tr("Failed to prepare delete oldest query: %1").arg(query.lastError().text()));
            db.rollback();
            emit historyChanged();
            return;
        }

        query.bindValue(":limit", MAX_HISTORY_ITEMS);
        if (!executeQuery(query, deleteOldestQuery, "delete oldest history item")) {
            qWarning() << "[HistoryState] Failed to execute delete oldest query.";
            db.rollback();
            emit historyChanged();
            return;
        }
    }

    // Commit the transaction
    if (!db.commit()) {
        qWarning() << "[HistoryState] Failed to commit database transaction:" << db.lastError().text();
        emit databaseError(tr("Failed to commit database transaction: %1").arg(db.lastError().text()));
        db.rollback();
    }

    emit historyChanged();
}

void HistoryState::clearHistory() {
    m_history.clear();

    QSqlDatabase db = QSqlDatabase::database("history_db");
    if (!db.isOpen()) {
        qWarning() << "[HistoryState] Database is not open";
        emit databaseError(tr("Database is not open"));
        emit historyChanged();
        return;
    }

    QSqlQuery query(db);
    QString clearQuery = "DELETE FROM history";
    if (!executeQuery(query, clearQuery, "clear history")) {
        qWarning() << "[HistoryState] Failed to execute clear history query.";
        emit historyChanged();
        return;
    }
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

bool HistoryState::executeQuery(QSqlQuery &query, const QString &queryText, const QString &operationDescription) {

    if (!query.exec()) {
        qWarning() << "[HistoryState] Failed to" << operationDescription << ":" << query.lastError().text();
        emit databaseError(tr("Failed to %1: %2. Query: %3")
                         .arg(operationDescription)
                         .arg(query.lastError().text())
                         .arg(queryText));
        return false;
    }
    return true;
}
