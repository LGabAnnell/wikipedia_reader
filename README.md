# Wikipedia Qt Client - Expanded Functionality

This project provides an expanded Wikipedia client for Qt applications with the following features:

## Features

### 1. Search Functionality
Search for Wikipedia articles by keyword:

```cpp
Wikipedia::WikipediaClient client;
connect(&client, &Wikipedia::WikipediaClient::searchCompleted, 
        this, &MyClass::handleSearchResults);
client.search("Quantum Computing", 10); // Search with limit of 10 results
```

### 2. Fetch Page Content
Get Wikipedia page content by title or page ID:

```cpp
// By title
client.getPage("Albert Einstein");

// By page ID
client.getPageById(736); // Einstein's page ID
```

### 3. Fetch Page with Images
Get Wikipedia page content including image URLs:

```cpp
connect(&client, &Wikipedia::WikipediaClient::pageWithImagesReceived, 
        this, &MyClass::handlePageWithImages);
client.getPageWithImages(736);
```

### 4. Featured Article of the Day
Fetch Wikipedia's featured article for the current day:

```cpp
connect(&client, &Wikipedia::WikipediaClient::featuredArticleReceived, 
        this, &MyClass::handleFeaturedArticle);
client.getFeaturedArticleOfTheDay();
```

## Data Structures

### SearchResult
```cpp
struct SearchResult {
    QString title;     // Article title
    QString snippet;   // Short description snippet
    int pageid;        // Wikipedia page ID
};
```

### Page
```cpp
struct Page {
    QString title;         // Article title
    QString extract;       // Article content extract
    int pageid;            // Wikipedia page ID
    QStringList imageUrls; // List of image URLs (when using getPageWithImages)
};
```

### FeaturedArticle
```cpp
struct FeaturedArticle {
    QString title;         // Article title
    QString extract;       // Article content extract
    int pageid;            // Wikipedia page ID
    QStringList imageUrls; // List of image URLs
    QString date;          // Date of the featured article
};
```

## Signals

- `searchCompleted(const QVector<SearchResult> &results)` - Emitted when search completes
- `pageReceived(const Page &page)` - Emitted when page content is received
- `pageWithImagesReceived(const Page &page)` - Emitted when page with images is received
- `featuredArticleReceived(const FeaturedArticle &article)` - Emitted when featured article is received
- `errorOccurred(const QString &error)` - Emitted when an error occurs

## Testing

The project includes comprehensive tests in the `test/` directory. Run tests with:

```bash
cd build
ctest
```

## Example Usage

See `examples/wikipedia_client_usage_example.cpp` for a complete example demonstrating all features.

## API Notes

- The client uses `test.wikipedia.org` by default for testing purposes
- For production use, you may want to change the base URL to `https://en.wikipedia.org/w/api.php`
- Image URL fetching uses a synchronous approach for simplicity - consider using async for production

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Running Tests

```bash
cd build
./test/test_wikipedia_client
```

## Dependencies

- Qt 6.8+ (Core, Network, Test components)
- C++20 compiler
