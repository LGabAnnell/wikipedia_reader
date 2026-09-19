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

    TestCase {
        name: "ApplicationModuleSmoke"

        function test_state_singletons_are_usable() {
            verify(GlobalState !== null && GlobalState !== undefined,
                   "GlobalState singleton must resolve from wikipedia_qt");
            verify(HistoryState !== null && HistoryState !== undefined,
                   "HistoryState singleton must resolve from wikipedia_qt");
            verify(NavigationState !== null && NavigationState !== undefined,
                   "NavigationState singleton must resolve from wikipedia_qt");

            compare(GlobalState.isLoading, false, "GlobalState must start idle");
            compare(GlobalState.currentPageTitle, "", "GlobalState must start without a page");
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
