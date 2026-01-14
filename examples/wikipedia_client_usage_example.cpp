// Example usage of the expanded WikipediaClient

#include <QCoreApplication>
#include <QDebug>
#include "../src/wikipedia_client/wikipedia_client.h"

class WikipediaClientExample : public QObject
{
    Q_OBJECT

public:
    WikipediaClientExample() : client(this) {
        // Connect signals to slots
        connect(&client, &Wikipedia::WikipediaClient::searchCompleted, 
                this, &WikipediaClientExample::handleSearchResults);
        connect(&client, &Wikipedia::WikipediaClient::pageReceived, 
                this, &WikipediaClientExample::handlePageReceived);
        connect(&client, &Wikipedia::WikipediaClient::pageWithImagesReceived, 
                this, &WikipediaClientExample::handlePageWithImagesReceived);
        connect(&client, &Wikipedia::WikipediaClient::featuredArticleReceived, 
                this, &WikipediaClientExample::handleFeaturedArticleReceived);
        connect(&client, &Wikipedia::WikipediaClient::errorOccurred, 
                this, &WikipediaClientExample::handleError);
    }

    void runExamples() {
        qDebug() << "=== Wikipedia Client Usage Examples ===";
        
        // Example 1: Search for articles
        qDebug() << "\n1. Searching for 'Quantum Computing'...";
        client.search("Quantum Computing", 5);
        
        // Example 2: Get page by ID (after a short delay)
        QTimer::singleShot(2000, this, [this]() {
            qDebug() << "\n2. Getting page by ID (Einstein's page: 736)...";
            client.getPageById(736);
        });
        
        // Example 3: Get page with images (after another delay)
        QTimer::singleShot(4000, this, [this]() {
            qDebug() << "\n3. Getting page with images (Einstein's page: 736)...";
            client.getPageWithImages(736);
        });
        
        // Example 4: Get featured article of the day (after another delay)
        QTimer::singleShot(6000, this, [this]() {
            qDebug() << "\n4. Getting featured article of the day...";
            client.getFeaturedArticleOfTheDay();
            
            // Quit the application after a final delay
            QTimer::singleShot(3000, qApp, &QCoreApplication::quit);
        });
    }

private slots:
    void handleSearchResults(const QVector<Wikipedia::SearchResult> &results) {
        qDebug() << "Search Results:";
        for (const auto &result : results) {
            qDebug() << "  - " << result.title << "(ID:" << result.pageid << ")";
            qDebug() << "    " << result.snippet;
        }
    }

    void handlePageReceived(const Wikipedia::Page &page) {
        qDebug() << "Page Received:";
        qDebug() << "  Title: " << page.title;
        qDebug() << "  Page ID: " << page.pageid;
        qDebug() << "  Extract (first 100 chars): " << page.extract.left(100) << "...";
    }

    void handlePageWithImagesReceived(const Wikipedia::Page &page) {
        qDebug() << "Page with Images Received:";
        qDebug() << "  Title: " << page.title;
        qDebug() << "  Page ID: " << page.pageid;
        qDebug() << "  Number of images: " << page.imageUrls.size();
        if (!page.imageUrls.isEmpty()) {
            qDebug() << "  First image URL: " << page.imageUrls.first();
        }
    }

    void handleFeaturedArticleReceived(const Wikipedia::FeaturedArticle &article) {
        qDebug() << "Featured Article Received:";
        qDebug() << "  Title: " << article.title;
        qDebug() << "  Date: " << article.date;
        qDebug() << "  Page ID: " << article.pageid;
        qDebug() << "  Number of images: " << article.imageUrls.size();
        if (!article.imageUrls.isEmpty()) {
            qDebug() << "  First image URL: " << article.imageUrls.first();
        }
    }

    void handleError(const QString &error) {
        qDebug() << "Error occurred: " << error;
    }

private:
    Wikipedia::WikipediaClient client;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // Register custom types for Qt's meta-object system
    qRegisterMetaType<Wikipedia::SearchResult>();
    qRegisterMetaType<Wikipedia::Page>();
    qRegisterMetaType<Wikipedia::FeaturedArticle>();
    qRegisterMetaType<QVector<Wikipedia::SearchResult>>();

    WikipediaClientExample example;
    example.runExamples();
    
    return app.exec();
}

#include "wikipedia_client_usage_example.moc"
