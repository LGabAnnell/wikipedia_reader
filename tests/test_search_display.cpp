// Test file to verify search results display functionality
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QVector>
#include "SearchState.h"
#include "SearchBarModel.h"
#include "wikipedia_search_client.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    if (!qgetenv("RUN_NETWORK_TESTS").toInt()) {
        qDebug() << "Network tests disabled. Set RUN_NETWORK_TESTS=1 to enable.";
        return 0;
    }

    // Create SearchState
    SettingsState settings;
    SearchState searchState(settings);

    // Create SearchBarModel — it auto-connects to SearchState::instance()
    SearchBarModel searchBarModel;

    // Connect to signals to monitor changes
    QObject::connect(&searchState, &SearchState::searchResultsChanged, [&]() {
        qDebug() << "Search results updated! Count:" << searchState.searchResults().count();
        for (const auto &result : searchState.searchResults()) {
            qDebug() << "  - Title:" << result.title;
            qDebug() << "    Snippet:" << result.snippet;
            qDebug() << "    Page ID:" << result.pageid;
        }
    });

    QObject::connect(&searchState, &SearchState::isSearchingChanged, [&]() {
        qDebug() << "Loading state changed:" << searchState.isSearching();
    });

    QObject::connect(&searchState, &SearchState::errorMessageChanged, [&]() {
        if (!searchState.errorMessage().isEmpty()) {
            qDebug() << "Error occurred:" << searchState.errorMessage();
        }
    });

    // Test search
    qDebug() << "Performing search for 'Quantum Computing'...";
    searchBarModel.setSearchText("Quantum Computing");
    searchBarModel.performSearch();

    // Quit after 10 seconds
    QTimer::singleShot(10000, &app, &QCoreApplication::quit);

    return app.exec();
}