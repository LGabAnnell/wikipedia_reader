#include "HistoryState.h"
#include "HistoryDatabase.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QSqlQuery>

class HistoryStateTest : public QObject {
    Q_OBJECT
    QTemporaryDir directory;
    bool sql(const QString &statement) {
        QSqlQuery query(QSqlDatabase::database("history_db"));
        return query.exec(statement);
    }
    QStringList snapshot(const QVector<history_item> &items) {
        QStringList result;
        for (const auto &item : items)
            result << QString::number(item.pageId) + item.language + item.title + item.timestamp.toString(Qt::ISODateWithMs);
        return result;
    }
private slots:
    void initTestCase() {
        QVERIFY(directory.isValid());
        qputenv("XDG_DATA_HOME", directory.path().toUtf8());
        QCoreApplication::setApplicationName("HistoryStateTest");
    }
    void cleanup() {
        QFile::remove(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/history.db");
    }
    void retentionAndReopen() {
        {
            HistoryState state;
            QVERIFY(state.databaseReady());
            for (int i = 1; i <= 65; ++i)
                state.addToHistory(QString::number(i), i, "en");
            QCOMPARE(state.history().size(), 50);
            QCOMPARE(state.history().first().pageId, 65);
            QCOMPARE(state.history().last().pageId, 16);
            QSqlQuery count(QSqlDatabase::database("history_db"));
            QVERIFY(count.exec("SELECT count(*) FROM history"));
            QVERIFY(count.next());
            QCOMPARE(count.value(0).toInt(), 50);
        }
        HistoryState state;
        QCOMPARE(state.history().size(), 50);
        QCOMPARE(state.history().last().pageId, 16);
    }
    void startupPruningAndTies() {
        {
            HistoryDatabase db;
            QVERIFY(db.initialize());
            for (int i = 1; i <= 60; ++i)
                QVERIFY(sql(QString("INSERT INTO history(page_id,title,timestamp) VALUES(%1,'Legacy','2024-01-01')").arg(i)));
        }
        HistoryState state;
        QVERIFY(state.databaseReady());
        QCOMPARE(state.history().size(), 50);
        QCOMPARE(state.history().first().pageId, 60);
        QCOMPARE(state.history().last().pageId, 11);
        QVERIFY(state.history().first().language.isEmpty());
        state.addToHistory("English", 60, "en");
        state.addToHistory("French", 60, "fr");
        QCOMPARE(state.history().size(), 50);
        QCOMPARE(state.history()[1].language, QString("en"));
    }
    void mutationFailures_data() {
        QTest::addColumn<QString>("operation");
        QTest::newRow("insert") << "INSERT";
        QTest::newRow("prune") << "DELETE";
        QTest::newRow("clear") << "CLEAR";
    }
    void mutationFailures() {
        QFETCH(QString, operation);
        HistoryState state;
        for (int i = 1; i <= 50; ++i)
            state.addToHistory(QString::number(i), i, "en");
        const auto before = snapshot(state.history());
        QSignalSpy changed(&state, &HistoryState::historyChanged);
        QSignalSpy errors(&state, &HistoryState::databaseError);
        QVERIFY(sql("CREATE TRIGGER fail BEFORE " + (operation == "INSERT" ? QString("INSERT") : QString("DELETE")) +
                    " ON history BEGIN SELECT RAISE(ABORT, 'injected failure'); END"));
        if (operation == "CLEAR")
            state.clearHistory();
        else
            state.addToHistory("Changed", operation == "INSERT" ? 50 : 51, "en");
        QCOMPARE(changed.count(), 0);
        QCOMPARE(errors.count(), 1);
        QVERIFY(state.databaseReady());
        QVERIFY(state.lastDatabaseError().contains("injected failure"));
        QCOMPARE(snapshot(state.history()), before);
        QSqlQuery rows(QSqlDatabase::database("history_db"));
        QVERIFY(rows.exec("SELECT page_id,title,timestamp,language FROM history ORDER BY timestamp DESC,id DESC"));
        QVector<history_item> stored;
        while (rows.next()) {
            history_item item;
            item.pageId = rows.value(0).toInt(); item.title = rows.value(1).toString();
            item.timestamp = rows.value(2).toDateTime(); item.language = rows.value(3).toString();
            stored.append(item);
        }
        rows.finish();
        QCOMPARE(snapshot(stored), before);
        QVERIFY(sql("DROP TRIGGER fail"));
        if (operation == "CLEAR") state.clearHistory();
        else state.addToHistory("Changed", 50, "en");
        QCOMPARE(changed.count(), 1);
        QVERIFY(state.lastDatabaseError().isEmpty());
        if (operation != "CLEAR") QCOMPARE(state.history().first().title, QString("Changed"));
    }
    void startupFailure_data() {
        QTest::addColumn<bool>("unsupported");
        QTest::newRow("schema") << true;
        QTest::newRow("pruning") << false;
    }
    void startupFailure() {
        QFETCH(bool, unsupported);
        {
            HistoryDatabase db;
            QVERIFY(db.initialize());
            if (unsupported) QVERIFY(sql("PRAGMA user_version=99"));
            else {
                for (int i = 0; i < 51; ++i)
                    QVERIFY(sql(QString("INSERT INTO history(page_id,title,timestamp) VALUES(%1,'Legacy','2024-01-01')").arg(i)));
                QVERIFY(sql("CREATE TRIGGER fail BEFORE DELETE ON history BEGIN SELECT RAISE(ABORT,'startup retention failure'); END"));
            }
        }
        HistoryState state;
        QVERIFY(!state.databaseReady());
        const QString error = state.lastDatabaseError();
        QVERIFY(error.contains(unsupported ? "99" : "startup retention failure"));
        state.clearHistory(); state.addToHistory("Ignored", 1, "en");
        QCOMPARE(state.lastDatabaseError(), error);
    }
    void identicalTimestampRetentionAndRevisit() {
        HistoryDatabase db;
        QVERIFY(db.initialize());
        const QDateTime timestamp(QDate(2024, 1, 1), QTime(12, 0));
        QVector<history_item> retained;
        for (int i = 1; i <= 60; ++i)
            QVERIFY(db.addToHistory(QString::number(i), i, "en", timestamp, &retained));
        QCOMPARE(retained.size(), 50);
        QCOMPARE(retained.first().pageId, 60);
        QCOMPARE(retained.last().pageId, 11);
        QCOMPARE(snapshot(db.loadHistory(100)), snapshot(retained));
        QVERIFY(db.addToHistory("Updated", 11, "en", timestamp.addSecs(1), &retained));
        QCOMPARE(retained.size(), 50);
        QCOMPARE(retained.first().title, QString("Updated"));
        QCOMPARE(retained.first().timestamp, timestamp.addSecs(1));
        QVERIFY(db.addToHistory("French", 11, "fr", timestamp.addSecs(2), &retained));
        QCOMPARE(retained[0].language, QString("fr"));
        QCOMPARE(retained[1].language, QString("en"));
    }
    void checkedLoadingAndOutputPreservation() {
        HistoryDatabase db;
        QVERIFY(db.initialize());
        QVector<history_item> items;
        QVERIFY(db.loadHistory(50, items));
        QVERIFY(items.isEmpty());
        QVERIFY(db.addToHistory("Original", 1, "en", QDateTime::currentDateTime(), &items));
        const auto before = snapshot(items);
        QVERIFY(sql("CREATE TRIGGER fail BEFORE INSERT ON history BEGIN SELECT RAISE(ABORT,'insert failure'); END"));
        QVERIFY(!db.addToHistory("Changed", 1, "en", QDateTime::currentDateTime(), &items));
        QCOMPARE(snapshot(items), before);
        QVERIFY(sql("DROP TABLE history"));
        QVERIFY(!db.loadHistory(50, items));
        QCOMPARE(snapshot(items), before);
    }
};
QTEST_GUILESS_MAIN(HistoryStateTest)
#include "HistoryStateTest.moc"
