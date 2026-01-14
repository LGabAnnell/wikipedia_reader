// Test file to verify the integration between SearchBarModel and GlobalState
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include "GlobalState.h"
#include "components/searchbar/SearchBarModel.h"
#include "wikipedia_client/wikipedia_client.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // Create GlobalState
    GlobalState globalState;

    // Create SearchBarModel and connect to GlobalState
    SearchBarModel searchBarModel;
    searchBarModel.setGlobalState(&globalState);

    // Test initial state
    qDebug() << "Initial state:";
    qDebug() << "  Search results count:" << globalState.searchResults().count();
    qDebug() << "  Is loading:" << globalState.isLoading();
    qDebug() << "  Current page title:" << globalState.currentPageTitle();

    // Set search text and perform search
    searchBarModel.setSearchText("Quantum Computing");
    searchBarModel.performSearch();

    // Connect to signals to monitor changes
    QObject::connect(&globalState, &GlobalState::searchResultsChanged, [&]() {
        qDebug() << "Search results updated! Count:" << globalState.searchResults().count();
        for (const auto &result : globalState.searchResults()) {
            qDebug() << "  -" << result.title;
        }
    });

    QObject::connect(&globalState, &GlobalState::isLoadingChanged, [&]() {
        qDebug() << "Loading state changed:" << globalState.isLoading();
    });

    QTimer::singleShot(5000, &app, &QCoreApplication::quit);

    return app.exec();
}
