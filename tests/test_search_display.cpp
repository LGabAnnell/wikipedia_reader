#include <QCoreApplication>
#include <QSignalSpy>
#include <QtTest/QtTest>

#include "SearchBarModel.h"
#include "SearchState.h"

class SearchDisplayLiveTest : public QObject {
    Q_OBJECT

private slots:
    void searchReturnsResults() {
        SettingsState settings;
        settings.setLanguage(QStringLiteral("en"));
        SearchState searchState(settings);
        SearchBarModel searchBarModel;
        QSignalSpy completed(&searchState, &SearchState::hasCompletedSearchChanged);
        QSignalSpy errors(&searchState, &SearchState::errorOccurred);

        searchBarModel.setSearchText(QStringLiteral("Quantum Computing"));
        searchBarModel.performSearch();
        QVERIFY(searchState.isSearching());
        QVERIFY(!searchState.hasCompletedSearch());

        QTRY_VERIFY_WITH_TIMEOUT(searchState.hasCompletedSearch() || !errors.isEmpty(), 45'000);
        QVERIFY2(errors.isEmpty(), qPrintable(errors.isEmpty() ? QString() : errors.first().first().toString()));
        QCOMPARE(completed.count(), 1);
        QVERIFY(!searchState.isSearching());
        QVERIFY(searchState.hasCompletedSearch());
        QVERIFY(!searchState.searchResults().isEmpty());
        for (const auto &result : searchState.searchResults()) {
            QVERIFY(!result.title.trimmed().isEmpty());
            QVERIFY(result.pageid > 0);
        }
    }
};

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    if (!qEnvironmentVariableIntValue("RUN_NETWORK_TESTS"))
        return 77;
    SearchDisplayLiveTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_search_display.moc"
