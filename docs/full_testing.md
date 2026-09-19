I recommend **Qt Quick Test with the real QML, models, and state objects, backed by a deterministic fake network
transport**. This would exercise the application’s actual signal connections, parsing, loading states, and navigation
without depending on Wikipedia availability.

No files were changed.

**Why this fits the current application**

Network clients are constructed independently
in [GlobalState.cpp](/home/gabriel/projects/wikipedia_reader/src/state/GlobalState.cpp), [SearchBarModel.cpp](/home/gabriel/projects/wikipedia_reader/src/modules/SearchBarModule/SearchBarModel.cpp),
and [HomeModel.cpp](/home/gabriel/projects/wikipedia_reader/src/modules/HomeModule/HomeModel.cpp). Home also fetches
data when its QML component finishes loading.

Consequently, replacing `GlobalState` with a QML mock would not isolate networking. C++ models also access
`GlobalState::instance()`, so a QML replacement could produce two different sources of state.

**1. Introduce one replaceable network boundary**

Give the Wikipedia clients a way to obtain their `QNetworkAccessManager` through an injectable factory. Production would
create ordinary managers; tests would create fake managers sharing a fixture controller. Install that factory before
constructing any state objects or QML models.

The fake manager would override `createRequest()` and return controlled `QNetworkReply` objects—an extension point
supported by [Qt’s networking API](https://doc.qt.io/qt-6/qnetworkaccessmanager.html#createRequest).

The fixture controller should:

- Match requests by host, path, and parsed query parameters.
- Return small, committed JSON/HTML fixtures.
- Hold requests pending until the test explicitly completes them.
- Simulate network errors, HTTP failures, empty results, and malformed responses.
- Record requests so tests can check page IDs, search terms, and language.
- Fail unexpected requests, with no fallback to the internet.

Replies must complete asynchronously, after the client has connected its signals. Explicit completion makes
loading-state tests predictable: submit search, verify disabled controls, release response, verify results.

I would choose this over introducing interfaces and fake implementations for all four clients initially. It preserves
the existing parsing and signal behavior, at the cost of a small, carefully implemented fake-reply helper.

**2. Add a dedicated Quick Test runner**

Create a test executable under `BUILD_TESTING`, linking `Qt6::QuickTest` and the application’s actual QML modules. Use
`QUICK_TEST_MAIN_WITH_SETUP` to initialize state, register singletons, install the SVG image provider, and expose a
test-only fixture controller. Qt documents this setup mechanism and `tst_*.qml` discovery in
its [Qt 6.8 Quick Test guide](https://doc.qt.io/qt-6.8/qtquicktest-index.html).

Move shared application initialization and necessary backend sources into reusable build targets so the application and
runner use the same registrations and resources.

One important detail: Quick Test creates an engine per test file, while `qmlRegisterSingletonInstance()` restricts the
instance to one engine. Initially, **register each QML test file as a separate CTest invocation using `-input`**. That
preserves the current singleton architecture and provides process
isolation. [Qt singleton registration documentation](https://doc.qt.io/qt-6/qqml-h.html#qmlRegisterSingletonInstance)

**3. Cover other sources of nondeterminism**

- Use a temporary history database, ideally through an injectable database path. Each process needs its own location.
- Replace fixture image URLs with bundled images. QML image loading uses a separate networking path; intercept or reject
  remaining remote requests through the QML engine’s network-manager factory.
- Provide a fixed date for home-feed scenarios, or explicitly match date-bearing requests without relying on today’s
  content.
- Reset pending replies, history, article cache, and navigation between cases; alternatively, give complex scenarios
  separate files/processes.

**4. Start with these behaviors**

| Area           | First useful tests                                                                                |
|----------------|---------------------------------------------------------------------------------------------------|
| Search         | Empty input, Enter/button submission, loading controls, populated/empty results, failure recovery |
| Article        | Result click loads the correct article; title and content update; errors clear loading            |
| Navigation     | Back restores the previous view; Home returns to root                                             |
| Sections       | Sections load, selection scrolls to the expected heading                                          |
| Home           | Fixture data appears; partial failures leave successful sections usable                           |
| Images/history | Local image selection and captions; article visits update isolated history                        |

Use real keyboard/mouse interaction, stable `objectName` values for important controls, and `tryCompare`/`tryVerify` or
`SignalSpy` for asynchronous assertions. Avoid fixed sleeps and checks against private Qt implementation class
names. [Qt TestCase documentation](https://doc.qt.io/qt-6/qml-qttest-testcase.html)

**5. Keep the test layers complementary**

Retain the existing C++ parser, state, and database tests. Add Quick Test for user-visible behavior. Keep live Wikipedia
checks separately labelled and opt-in, building on the existing `RUN_NETWORK_TESTS` convention.

For CI, start with offscreen execution and software rendering; use Xvfb for tests that require normal window exposure or
focus behavior.

I would implement this incrementally: first the runner and transport injection, then one complete search → article →
back scenario. That would validate the architecture before extending coverage across the remaining screens.

**6. next steps**
The next step would be to add more QML tests to cover the user-visible behaviors listed in §4 of docs/full_testing.md,
using the existing harness:
• Create new tst_*.qml files under tests/qml/ for each major area (search, article, navigation, sections, home,
images/history).
• Register fixtures for the API calls your new tests will make (e.g., search results, article content, section list,
home feed items, image metadata).
• Write TestCase blocks that:
▪ Trigger user actions (e.g., search input, button click, item selection).
▪ Assert loading states, navigation stack changes, and UI updates.
▪ Verify no stray network requests or QML warnings (testSupport.requestCount, testSupport.qmlWarningCount).
• Run the new tests with ./scripts/run_tests.sh; each file will be auto-registered as a new CTest entry
(QmlTests.<name>).
Start with a single, complete scenario (e.g., search → article → back navigation) to validate the architecture before
extending coverage across the remaining screens.