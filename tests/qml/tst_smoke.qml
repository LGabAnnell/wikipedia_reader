import QtQuick
import QtTest
import wikipedia_qt
import wikipedia_qt.Sidebar 1.0

Item {
    id: root
    width: 400
    height: 300

    Component {
        id: sidebarComponent

        Sidebar {
            objectName: "applicationSidebar"
        }
    }

    Component {
        id: modelTypesComponent

        QtObject {
            property search_result searchResult
            property page articlePage
            property featured_article featuredArticle
            property news_item newsItem
            property on_this_day_event onThisDayEvent
            property did_you_know_item didYouKnowItem
            property history_item historyItem
            property section articleSection
        }
    }

    TestCase {
        name: "ApplicationModuleSmoke"

        function test_backend_value_types_are_registered() {
            const models = createTemporaryObject(modelTypesComponent, root);
            verify(models !== null, "All backend value types must resolve from wikipedia_qt");
            compare(models.searchResult.title, "");
            compare(models.articlePage.title, "");
            compare(models.featuredArticle.title, "");
            compare(models.newsItem.title, "");
            compare(models.onThisDayEvent.event, "");
            compare(models.didYouKnowItem.text, "");
            compare(models.historyItem.title, "");
            compare(models.articleSection.title, "");
            compare(testSupport.requestCount, 0);
            compare(testSupport.qmlWarningCount, 0, testSupport.qmlWarnings.join("\n"));
        }

        function test_state_singletons_are_usable() {
            verify(ArticleState !== null && ArticleState !== undefined,
                   "ArticleState singleton must resolve from wikipedia_qt");
            verify(HistoryState !== null && HistoryState !== undefined,
                   "HistoryState singleton must resolve from wikipedia_qt");
            verify(NavigationState !== null && NavigationState !== undefined,
                   "NavigationState singleton must resolve from wikipedia_qt");

            compare(ArticleState.isLoading, false, "ArticleState must start idle");
            compare(ArticleState.currentPageTitle, "", "ArticleState must start without a page");
            compare(SettingsState.language, "en");
            compare(ImageSelectionState.currentImageUrl, "");
            compare(ImageSelectionState.currentImageDescription, "");
            compare(SearchState.isSearching, false);
            compare(SearchState.hasCompletedSearch, false);
            compare(SearchState.searchResults.length, 0);
            compare(SearchState.errorMessage, "");
            verify(ClipboardHelper !== null && ClipboardHelper !== undefined);
            compare(HistoryState.history.length, 0, "HistoryState must start empty");
            compare(NavigationState.stackView, null, "NavigationState must start without a stack view");

            compare(testSupport.requestCount, 0, "Singleton checks must not make network requests");
            compare(testSupport.qmlWarningCount, 0, testSupport.qmlWarnings.join("\n"));
        }

        function test_sidebar_loads_without_network_or_warnings() {
            const sidebar = sidebarComponent.createObject(root, {
                width: root.width,
                height: root.height,
                searchResults: []
            });

            verify(sidebar !== null, "Sidebar from wikipedia_qt.Sidebar must be created");
            compare(sidebar.objectName, "applicationSidebar");
            compare(testSupport.requestCount, 0, "Smoke test must not make network requests");
            compare(testSupport.qmlWarningCount, 0, testSupport.qmlWarnings.join("\n"));

            sidebar.destroy();
        }
    }
}
