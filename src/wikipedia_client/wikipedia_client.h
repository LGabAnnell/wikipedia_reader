// Wikipedia API C++ Client Outline
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <ranges>
#include <regex>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Wikipedia
{

    // Structure to represent a Wikipedia search result
    struct SearchResult
    {
        std::string title;
        std::string snippet;
        int pageid;
    };

    // Structure to represent a Wikipedia page
    struct Page
    {
        std::string title;
        std::string extract;
        int pageid;
    };

    class WikipediaClient
    {
    public:
        WikipediaClient();
        ~WikipediaClient();

        // Search Wikipedia
        std::vector<SearchResult> search(const std::string &query, int limit = 10);

        // Get page content by title
        Page getPage(const std::string &title);

        // Get page content by ID
        Page getPageById(int pageid);

    private:
        CURL *curl;
        std::string baseUrl;

        // Helper function to perform HTTP GET request
        std::string httpGet(const std::string &url);
    };

} // namespace Wikipedia
