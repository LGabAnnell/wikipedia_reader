// WikipediaClient.cpp
#include "wikipedia_client.h"

namespace Wikipedia
{

    WikipediaClient::WikipediaClient() : curl(curl_easy_init()), baseUrl("https://en.wikipedia.org/w/api.php")
    {
        if (!curl)
        {
            throw std::runtime_error("Failed to initialize CURL");
        }
    }

    WikipediaClient::~WikipediaClient()
    {
        if (curl)
        {
            curl_easy_cleanup(curl);
        }
    }

    size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *output)
    {
        size_t total_size = size * nmemb;
        output->append((char *)contents, total_size);
        return total_size;
    }

    std::string WikipediaClient::httpGet(const std::string &url)
    {
        std::string readBuffer;
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "WikipediaClient/1.0");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            throw std::runtime_error(curl_easy_strerror(res));
        }
        return readBuffer;
    }

    std::vector<SearchResult> WikipediaClient::search(const std::string &query, int limit)
    {
        std::string encodedQuery = curl_easy_escape(curl, query.c_str(), query.length());
        std::string url = baseUrl + "?action=query&format=json&list=search&srsearch=" + encodedQuery + "&srlimit=" + std::to_string(limit);
        std::string response = httpGet(url);
        json j = json::parse(response);

        std::vector<SearchResult> results;
        for (auto &item : j["query"]["search"])
        {
            results.push_back({item["title"],
                               item["snippet"],
                               item["pageid"]});
        }
        return results;
    }

    Page WikipediaClient::getPage(const std::string &title)
    {
        std::string url = baseUrl + "?action=query&format=json&prop=extracts&titles=" + title + "&explaintext=1";
        std::string response = httpGet(url);
        json j = json::parse(response);

        auto pages = j["query"]["pages"];
        for (auto it = pages.begin(); it != pages.end(); ++it)
        {
            if (it.value()["title"] == title)
            {
                return {
                    it.value()["title"],
                    it.value()["extract"],
                    std::stoi(it.key())};
            }
        }
        throw std::runtime_error("Page not found");
    }

    Page WikipediaClient::getPageById(int pageid)
    {
        std::string url = baseUrl + "?action=query&format=json&prop=extracts&pageids=" + std::to_string(pageid) + "&explaintext=1";
        std::string response = httpGet(url);
        json j = json::parse(response);

        auto pages = j["query"]["pages"];
        for (auto it = pages.begin(); it != pages.end(); ++it)
        {
            if (std::stoi(it.key()) == pageid)
            {
                return {
                    it.value()["title"],
                    it.value()["extract"],
                    std::stoi(it.key())};
            }
        }
        throw std::runtime_error("Page not found");
    }

} // namespace Wikipedia
