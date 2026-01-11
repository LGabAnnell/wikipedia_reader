#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "wikipedia_client/wikipedia_client.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []()
        { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("wikipedia_qt", "Main");

    Wikipedia::WikipediaClient client;

    // Search for articles
    auto results = client.search("C++ programming language");
    for (const auto &result : results)
    {
        std::cout << "Title: " << result.title << "\nSnippet: " << result.snippet << "\nID: " << result.pageid << "\n\n";
    }

    // Get page content
    try
    {
        auto page = client.getPage("C++");
        std::cout << "Title: " << page.title << "\nContent: " << page.extract.substr(0, 200) << "...\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return app.exec();
}
