// Test file to verify search results display functionality
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

    // Connect to signals to monitor changes
    QObject::connect(&globalState, &GlobalState::searchResultsChanged, [&]() {
        qDebug() << "Search results updated! Count:" << globalState.searchResults().count();
        for (const auto &result : globalState.searchResults()) {
            qDebug() << "  - Title:" << result.title;
            qDebug() << "    Snippet:" << result.snippet;
            qDebug() << "    Page ID:" << result.pageid;
        }
    });

    QObject::connect(&globalState, &GlobalState::isLoadingChanged, [&]() {
        qDebug() << "Loading state changed:" << globalState.isLoading();
    });

    QObject::connect(&globalState, &GlobalState::errorMessageChanged, [&]() {
        if (!globalState.errorMessage().isEmpty()) {
            qDebug() << "Error occurred:" << globalState.errorMessage();
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