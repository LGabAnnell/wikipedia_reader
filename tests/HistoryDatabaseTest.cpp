// tests/HistoryDatabaseTest.cpp
#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QDir>
#include <QStandardPaths>
#include "HistoryDatabase.h"

class HistoryDatabaseTest : public QObject {
    Q_OBJECT

private:
    HistoryDatabase *m_db = nullptr;
    QSignalSpy *m_initSpy = nullptr;
    QSignalSpy *m_errorSpy = nullptr;

private slots:
    void initTestCase() {
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

        // Clear any leftover data from previous test
        m_db->clearHistory();
    }

    void cleanup() {
        delete m_errorSpy;
        m_errorSpy = nullptr;
        delete m_initSpy;
        m_initSpy = nullptr;
        delete m_db;
        m_db = nullptr;
    }

    void cleanupTestCase() {
        // Remove the test database file
        QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().remove(dataPath + "/history.db");
    }

    void testInitialize() {
        // The databaseInitialized signal should have been emitted in the constructor
        // Wait a moment for the signal (it's emitted synchronously in initialize())
        QVERIFY(m_db != nullptr);
        QVERIFY(QSqlDatabase::contains("history_db"));
        QVERIFY(QSqlDatabase::database("history_db").isOpen());
    }

    void testAddAndLoad() {
        QDateTime ts = QDateTime::currentDateTime();
        QVERIFY(m_db->addToHistory("Test Article", 100, ts));

        QVector<history_item> history = m_db->loadHistory(50);
        QCOMPARE(history.size(), 1);
        QCOMPARE(history[0].title, QString("Test Article"));
        QCOMPARE(history[0].pageId, 100);
    }

    void testAddMultiple() {
        for (int i = 1; i <= 3; ++i) {
            QVERIFY(m_db->addToHistory(QString("Article %1").arg(i), i * 100,
                                       QDateTime::currentDateTime().addSecs(i)));
        }

        QVector<history_item> history = m_db->loadHistory(50);
        QCOMPARE(history.size(), 3);
    }

    void testClearHistory() {
        m_db->addToHistory("A", 1, QDateTime::currentDateTime());
        m_db->addToHistory("B", 2, QDateTime::currentDateTime());

        QVERIFY(m_db->clearHistory());

        QVector<history_item> history = m_db->loadHistory(50);
        QVERIFY(history.isEmpty());
    }

    void testDuplicatePageId() {
        QDateTime ts1 = QDateTime::currentDateTime();
        m_db->addToHistory("First Title", 500, ts1);

        QDateTime ts2 = QDateTime::currentDateTime().addSecs(1);
        m_db->addToHistory("Second Title", 500, ts2);

        QVector<history_item> history = m_db->loadHistory(50);
        // The delete-then-insert logic should ensure only 1 entry for pageId 500
        QCOMPARE(history.size(), 1);
        QCOMPARE(history[0].title, QString("Second Title"));
        QCOMPARE(history[0].pageId, 500);
    }

    void testRemoveOldest() {
        for (int i = 1; i <= 5; ++i) {
            m_db->addToHistory(QString("Article %1").arg(i), i,
                               QDateTime::currentDateTime().addSecs(i));
        }

        // Keep only the 3 most recent
        QVERIFY(m_db->removeOldestItems(3));

        QVector<history_item> history = m_db->loadHistory(50);
        QCOMPARE(history.size(), 3);
    }

    void testLoadWithLimit() {
        for (int i = 1; i <= 5; ++i) {
            m_db->addToHistory(QString("Article %1").arg(i), i,
                               QDateTime::currentDateTime().addSecs(i));
        }

        QVector<history_item> history = m_db->loadHistory(3);
        QCOMPARE(history.size(), 3);
    }

    void testTimestampOrdering() {
        // Add items with increasing timestamps
        for (int i = 1; i <= 3; ++i) {
            m_db->addToHistory(QString("Article %1").arg(i), i,
                               QDateTime(QDate(2024, 1, i), QTime(10, 0, 0)));
        }

        QVector<history_item> history = m_db->loadHistory(50);
        QCOMPARE(history.size(), 3);
        // loadHistory returns DESC by timestamp — most recent first
        QCOMPARE(history[0].title, QString("Article 3"));
        QCOMPARE(history[1].title, QString("Article 2"));
        QCOMPARE(history[2].title, QString("Article 1"));
    }

    void testDatabaseErrorSignal() {
        // The error spy should start empty if no errors occurred
        QVERIFY(m_errorSpy != nullptr);
        // We can't easily trigger a database error without corrupting the DB,
        // but we can verify the signal spy is set up correctly
        QCOMPARE(m_errorSpy->count(), 0);
    }
};

QTEST_MAIN(HistoryDatabaseTest)
#include "HistoryDatabaseTest.moc"
