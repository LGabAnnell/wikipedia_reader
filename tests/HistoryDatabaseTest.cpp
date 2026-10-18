// tests/HistoryDatabaseTest.cpp
#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QDir>
#include <QStandardPaths>
#include "HistoryDatabase.h"

class HistoryDatabaseTest : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dataDirectory;
    HistoryDatabase *m_db = nullptr;
    QSignalSpy *m_initSpy = nullptr;
    QSignalSpy *m_errorSpy = nullptr;

private slots:
    void initTestCase() {
        QVERIFY(m_dataDirectory.isValid());
        qputenv("XDG_DATA_HOME", m_dataDirectory.path().toUtf8());
        // Use a unique application name so the DB is isolated to this test
        QCoreApplication::setApplicationName("HistoryDatabaseTest");
    }

    void init() {
        // Only one HistoryDatabase instance can exist per process (fixed
        // connection name "history_db"). Create it fresh for each test and
        // clear any leftover data.
        m_db = new HistoryDatabase();
        m_initSpy = new QSignalSpy(m_db, &HistoryDatabase::databaseInitialized);
        m_errorSpy = new QSignalSpy(m_db, &HistoryDatabase::databaseError);

        QVERIFY(m_db->initialize());

        // Each case gets an independent schema as well as independent data.
        QVERIFY(m_db->clearHistory());
    }

    void cleanup() {
        delete m_errorSpy;
        m_errorSpy = nullptr;
        delete m_initSpy;
        m_initSpy = nullptr;
        delete m_db;
        m_db = nullptr;
        QFile::remove(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/history.db");
    }

    void cleanupTestCase() {
        // Remove the test database file
        QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().remove(dataPath + "/history.db");
    }

    void testInitialize() {
        // Initialization is explicit and observable.
        QCOMPARE(m_initSpy->count(), 1);
        QVERIFY(m_db != nullptr);
        QVERIFY(QSqlDatabase::contains("history_db"));
        QVERIFY(QSqlDatabase::database("history_db").isOpen());
    }

    void testAddAndLoad() {
        QDateTime ts = QDateTime::currentDateTime();
        QVERIFY(m_db->addToHistory("Test Article", 100, "en", ts));

        QVector<history_item> history = m_db->loadHistory(50);
        QCOMPARE(history.size(), 1);
        QCOMPARE(history[0].title, QString("Test Article"));
        QCOMPARE(history[0].pageId, 100);
    }

    void testAddMultiple() {
        for (int i = 1; i <= 3; ++i) {
            QVERIFY(m_db->addToHistory(QString("Article %1").arg(i), i * 100, "en",
                                       QDateTime::currentDateTime().addSecs(i)));
        }

        QVector<history_item> history = m_db->loadHistory(50);
        QCOMPARE(history.size(), 3);
    }

    void testClearHistory() {
        m_db->addToHistory("A", 1, "en", QDateTime::currentDateTime());
        m_db->addToHistory("B", 2, "fr", QDateTime::currentDateTime());
        {
            QSqlQuery query(QSqlDatabase::database("history_db"));
            QVERIFY(query.exec("INSERT INTO history(page_id,title,timestamp) VALUES(3,'Unknown edition','2024-01-01')"));
        }
        QCOMPARE(m_db->loadHistory(50).size(), 3);

        QVERIFY(m_db->clearHistory());

        QVector<history_item> history = m_db->loadHistory(50);
        QVERIFY(history.isEmpty());
    }

    void testDuplicatePageId() {
        QDateTime ts1 = QDateTime::currentDateTime();
        m_db->addToHistory("First Title", 500, "en", ts1);

        QDateTime ts2 = QDateTime::currentDateTime().addSecs(1);
        m_db->addToHistory("Second Title", 500, "en", ts2);

        QVector<history_item> history = m_db->loadHistory(50);
        // The delete-then-insert logic should ensure only 1 entry for pageId 500
        QCOMPARE(history.size(), 1);
        QCOMPARE(history[0].title, QString("Second Title"));
        QCOMPARE(history[0].pageId, 500);
    }

    void testRemoveOldest() {
        for (int i = 1; i <= 5; ++i) {
            m_db->addToHistory(QString("Article %1").arg(i), i, "en",
                               QDateTime::currentDateTime().addSecs(i));
        }

        // Keep only the 3 most recent
        QVERIFY(m_db->removeOldestItems(3));

        QVector<history_item> history = m_db->loadHistory(50);
        QCOMPARE(history.size(), 3);
    }

    void testLoadWithLimit() {
        for (int i = 1; i <= 5; ++i) {
            m_db->addToHistory(QString("Article %1").arg(i), i, "en",
                               QDateTime::currentDateTime().addSecs(i));
        }

        QVector<history_item> history = m_db->loadHistory(3);
        QCOMPARE(history.size(), 3);
    }

    void testTimestampOrdering() {
        // Add items with increasing timestamps
        for (int i = 1; i <= 3; ++i) {
            m_db->addToHistory(QString("Article %1").arg(i), i, "en",
                               QDateTime(QDate(2024, 1, i), QTime(10, 0, 0)));
        }

        QVector<history_item> history = m_db->loadHistory(50);
        QCOMPARE(history.size(), 3);
        // loadHistory returns DESC by timestamp — most recent first
        QCOMPARE(history[0].title, QString("Article 3"));
        QCOMPARE(history[1].title, QString("Article 2"));
        QCOMPARE(history[2].title, QString("Article 1"));
    }


    void testEditionIdentityPersistsAndIsUnique() {
        const QDateTime first(QDate(2024, 1, 1), QTime(12, 0));
        QVERIFY(m_db->addToHistory("English", 42, "en", first));
        QVERIFY(m_db->addToHistory("Français", 42, "fr", first.addSecs(1)));
        QVERIFY(m_db->addToHistory("English updated", 42, "en", first.addSecs(2)));
        delete m_db;
        m_db = new HistoryDatabase;
        QVERIFY(m_db->initialize());
        const auto items = m_db->loadHistory(50);
        QCOMPARE(items.size(), 2);
        QCOMPARE(items[0].language, QString("en"));
        QCOMPARE(items[0].title, QString("English updated"));
        QCOMPARE(items[0].timestamp, first.addSecs(2));
        QCOMPARE(items[1].language, QString("fr"));
        QCOMPARE(items[1].title, QString("Français"));
        QSqlQuery query(QSqlDatabase::database("history_db"));
        QVERIFY(!query.exec("INSERT INTO history(page_id,title,timestamp,language) VALUES(42,'duplicate','2024-01-01','en')"));
        QVERIFY(!m_db->addToHistory("Invalid", 43, "  ", first));
        QCOMPARE(m_db->loadHistory(50).size(), 2);
    }

    void testLegacyMigrationPreservesUnknownEditionAndRunsOnce() {
        QSqlDatabase db = QSqlDatabase::database("history_db");
        QSqlQuery query(db);
        QVERIFY(query.exec("DROP TABLE history"));
        QVERIFY(query.exec("CREATE TABLE history(id INTEGER PRIMARY KEY AUTOINCREMENT, page_id INTEGER NOT NULL, title TEXT NOT NULL, timestamp DATETIME NOT NULL)"));
        QVERIFY(query.exec("CREATE INDEX idx_page_id ON history(page_id)"));
        QVERIFY(query.exec("INSERT INTO history(id,page_id,title,timestamp) VALUES(17,42,'Legacy','2024-01-02T03:04:05.000')"));
        QVERIFY(query.exec("PRAGMA user_version = 0"));
        query = QSqlQuery();
        db = QSqlDatabase();
        delete m_db;
        m_db = new HistoryDatabase;
        QVERIFY(m_db->initialize());
        auto items = m_db->loadHistory(50);
        QCOMPARE(items.size(), 1);
        QCOMPARE(items[0].pageId, 42);
        QCOMPARE(items[0].title, QString("Legacy"));
        QCOMPARE(items[0].timestamp, QDateTime(QDate(2024, 1, 2), QTime(3, 4, 5)));
        QVERIFY(items[0].language.isEmpty());
        {
            QSqlQuery migrated(QSqlDatabase::database("history_db"));
            QVERIFY(migrated.exec("SELECT id, language FROM history"));
            QVERIFY(migrated.next());
            QCOMPARE(migrated.value(0).toInt(), 17);
            QVERIFY(migrated.value(1).isNull());
            QVERIFY(migrated.exec("PRAGMA user_version"));
            QVERIFY(migrated.next());
            QCOMPARE(migrated.value(0).toInt(), 1);
            QVERIFY(migrated.exec("SELECT name FROM sqlite_master WHERE name = 'idx_page_id'"));
            QVERIFY(!migrated.next());
        }
        delete m_db;
        m_db = new HistoryDatabase;
        QVERIFY(m_db->initialize());
        QCOMPARE(m_db->loadHistory(50).size(), 1);
        QVERIFY(m_db->addToHistory("Known English", 42, "en", QDateTime::currentDateTime()));
        QCOMPARE(m_db->loadHistory(50).size(), 2);
    }

    void testMigrationFailureRollsBackAndBlocksOperations() {
        {
            QSqlQuery query(QSqlDatabase::database("history_db"));
            QVERIFY(query.exec("DROP TABLE history"));
            QVERIFY(query.exec("CREATE TABLE history(id INTEGER PRIMARY KEY, page_id INTEGER, title TEXT, timestamp DATETIME)"));
            QVERIFY(query.exec("CREATE INDEX idx_page_id ON history(page_id)"));
            QVERIFY(query.exec("INSERT INTO history VALUES(17,42,'Legacy','2024-01-01')"));
            // An unrelated table occupying the new index name fails migration
            // after ALTER TABLE and DROP INDEX have already succeeded.
            QVERIFY(query.exec("CREATE TABLE idx_history_language_page_id(value TEXT)"));
            QVERIFY(query.exec("PRAGMA user_version = 0"));
        }
        delete m_db;
        m_db = new HistoryDatabase;
        QSignalSpy errors(m_db, &HistoryDatabase::databaseError);
        QVERIFY(!m_db->initialize());
        QVERIFY(!errors.isEmpty());
        QVERIFY(m_db->loadHistory(50).isEmpty());
        QVERIFY(!m_db->addToHistory("Blocked", 43, "en", QDateTime::currentDateTime()));
        QVERIFY(!m_db->clearHistory());
        QVERIFY(!m_db->removeOldestItems(0));
        QSqlQuery query(QSqlDatabase::database("history_db"));
        QVERIFY(query.exec("PRAGMA user_version"));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 0);
        QVERIFY(query.exec("SELECT id, title FROM history"));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 17);
        QCOMPARE(query.value(1).toString(), QString("Legacy"));
        QVERIFY(!query.exec("SELECT language FROM history"));
        QVERIFY(query.exec("SELECT name FROM sqlite_master WHERE name = 'idx_page_id'"));
        QVERIFY(query.next());
    }

    void testDatabaseErrorSignal() {
        // The error spy should start empty if no errors occurred
        QVERIFY(m_errorSpy != nullptr);
        // We can't easily trigger a database error without corrupting the DB,
        // but we can verify the signal spy is set up correctly
        QCOMPARE(m_errorSpy->count(), 0);
    }
};

QTEST_GUILESS_MAIN(HistoryDatabaseTest)
#include "HistoryDatabaseTest.moc"
