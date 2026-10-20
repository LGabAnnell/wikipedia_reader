# Testing

## Dependencies

Building the tests requires the application's C++20 toolchain, CMake 3.16+,
Qt 6.8+ (including Qt Test and Qt Quick Test), tinyxml2, and Lexbor 3.0+.
Article HTML processing uses Lexbor's HTML5 fragment parser and DOM;
HeaderModule still uses tinyxml2 for SVG icon recoloring.

For a local Lexbor installation:

```bash
git clone --branch v3.0.0 https://github.com/lexbor/lexbor.git
cmake -S lexbor -B lexbor/build -DCMAKE_INSTALL_PREFIX="$PWD/lexbor/install"
cmake --build lexbor/build -j14
cmake --install lexbor/build
```

Pass that installation prefix when configuring Wikipedia Reader so CMake can
find `lexbor-config.cmake`:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
    -DCMAKE_PREFIX_PATH="$PWD/lexbor/install"
```

## Run the tests

`BUILD_TESTING` defaults to `OFF`. Configure with it enabled, then build and
run CTest from the repository root. If dependencies are already discoverable:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build -j14
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
```

Use the prefix option above when Lexbor is installed locally.
`HistoryStateTest`, `HistoryDatabaseTest`, `WikipediaSearchClientTest`,
`WikipediaFeaturedClientTest`, and `WikipediaHomeClientTest` use
`QTEST_GUILESS_MAIN` and run without a display or GUI platform plugin.
Other C++ tests still use `QTEST_MAIN`; setting `QT_QPA_PLATFORM=offscreen`
for the full run supports headless execution. CTest already sets it individually
for QML tests, `ContentDisplayModelTest`, and `test_sidebar_layout`.

Database-using state tests, `HistoryDatabaseTest`, and the QML harness redirect
`XDG_DATA_HOME` to their own temporary directories. To also supply a writable
application-data location for the overall run:

```bash
test_data_dir=$(mktemp -d)
QT_QPA_PLATFORM=offscreen XDG_DATA_HOME="$test_data_dir" \
    ctest --test-dir build --output-on-failure
rm -r "$test_data_dir"
```

List registered tests, run QML tests as a group, or select individual entries:

```bash
ctest --test-dir build -N
ctest --test-dir build -R '^QmlTests\.' --output-on-failure
ctest --test-dir build -R '^QmlTests\.tst_article$' --output-on-failure
ctest --test-dir build -R '^QmlTests\.tst_feature_isolation$' --output-on-failure
ctest --test-dir build -R '^WikipediaRequestIsolationTest$' --output-on-failure
```

`scripts/run_tests.sh` configures when needed, builds incrementally, and runs
CTest with the offscreen platform. It accepts `-R <regex>`, `-n` for live
network testing, and `-r` to delete and rebuild `build/`. Configure local
dependency prefixes first. The separate root `test.sh` always deletes
`build/` before configuring and rebuilding.

The current source configuration registers **27 CTest entries: 16 C++ and
11 QML**. These are executables/test files, not individual test cases or
data rows. The inventory below reflects the sources reviewed on 2026-10-18;
it is not a record of a passing test run.

## C++ coverage

Each named entry below has a matching `.cpp` file under `tests/`.

| CTest entry | Coverage |
| --- | --- |
| `ArticleStateTest` | Initial state and notifications, HTML loading and caching, history on repeat visits, title resolution/cache hits, clearing old content before title loads, errors/retry, settings at construction, language-driven content/cache clearing, rejection of obsolete article/title replies, cached selections superseding pending loads, and singleton teardown/recreation. |
| `SearchStateTest` | Blank queries, trimming, pending/completed/reset state, duplicate submission while busy, errors/retry, settings at construction, restarting stored search text once after language changes, blank-query behavior, stale reply rejection, rapid language changes, and teardown/recreation. |
| `SettingsStateTest` | Default language, change notifications, ignoring an unchanged language, and singleton teardown/recreation. |
| `ImageSelectionStateTest` | URL/caption updated together before notifications, unchanged-selection notification suppression, clearing selection on language changes, and teardown/recreation. |
| `WikipediaRequestIsolationTest` | Obsolete search successes/errors and featured replies are ignored; old page and random-title replies cannot start follow-up HTML or summary requests after a language switch. |
| `WikipediaNetworkAccessManagerTest` | Both API-client and QML-image managers apply the runtime HTTP/2 policy: disabled for Qt 6.11.2, otherwise left at its default. Uses a temporary local file, not a live HTTP server. |
| `ContentDisplayModelTest` | Per-view section state, section-position/highlight resets, article-driven search/section resets, and case-insensitive text-search navigation wrapping. |
| `HtmlProcessorTest` | Lexbor-based cleanup and repaired HTML, style/script/link removal, legacy colors, figure/thumbnail conversion, captions and fallback descriptions, preserved surrounding text, inline/table/multiple images, math accessibility cleanup, entities, and URL normalization. |
| `HistoryStateTest` | Global 50-row retention and state recreation, startup pruning including legacy entries, identical-timestamp ordering, edition identity and revisits, trigger-injected insertion/pruning/clear rollback, unchanged memory and notifications on failure, retry recovery, unsupported-schema/startup-pruning errors, and checked-load/output preservation. |
| `HistoryDatabaseTest` | Database opens, add/load/clear, edition-aware identity and uniqueness across database reopen, legacy-schema migration and idempotence, migration-failure rollback and blocked operations, explicit pruning, load limits, and timestamp ordering. The separate generic error-signal test only checks that no error was emitted; migration tests do inject a failure. |
| `WikipediaSearchClientTest` | Search-result fields/counts, empty responses, missing keys, and malformed JSON. |
| `WikipediaPageClientTest` | Page fields, title/ID mismatch, image titles, sections, empty/missing sections, and malformed JSON. |
| `WikipediaFeaturedClientTest` | Featured parsing, feed thumbnail, missing `tfa`, invalid page IDs, malformed JSON, API errors, and unavailable versus invalid outcomes. |
| `WikipediaHomeClientTest` | News parsing and thumbnails, current most-read feeds without lookback, bounded lookback, news fallback after older-feed failures, typed section failures, invalid current feeds, on-this-day validation, and both Did-you-know request stages. |
| `test_sidebar_layout` | Sidebar creation, presence of a ListView, and the `searchResults` property. CTest supplies its working directory and QML import path. |
| `test_search_display` | Opt-in live search through `SearchBarModel` and `SearchState`; see the limitations below. |

The parser tests call parsing helpers directly; they do not establish complete
HTTP error handling. Some deliberately expect empty/default data for malformed
JSON or missing fields.

`ArticleStateTest`, `SearchStateTest`, and `WikipediaRequestIsolationTest` use
the shared fixture transport. Backend implementation and meta-objects are
linked through `wikipedia_qt_core`; fake-network support is compiled into
`wikipedia_test_network`. `ContentDisplayModelTest` links its feature module.
The search-display and sidebar-layout executables still compile their feature
models directly.

### Live search

```bash
RUN_NETWORK_TESTS=1 ctest --test-dir build -R '^test_search_display$' --output-on-failure
```

Without a nonzero integer `RUN_NETWORK_TESTS` value, this executable exits
successfully without exercising search. When enabled, it submits “Quantum
Computing,” logs state/results/errors, and exits after ten seconds. It has no
assertion that a response arrived or that search succeeded, so a passing CTest
result is not proof of a successful live search. The remaining tests use
fixtures, in-memory inputs, or local files rather than live Wikipedia requests.

## QML coverage

Each file under `tests/qml/` is registered as `QmlTests.<filename without .qml>`.

| Test file | Coverage |
| --- | --- |
| `tst_smoke.qml` | Backend value-type registration, usable state singletons and initial values, and Sidebar creation without requests or QML warnings. |
| `tst_search.qml` | Blank input, mouse and Enter submission, pending controls, populated/empty responses, and error recovery/retry. |
| `tst_article.qml` | Search-result selection, deferred loading and rendered HTML, inline image captions/fullscreen selection and Back, load errors without failed history visits, clearing visible and covered article views on language changes without changing stack depth, and article-link navigation preserving the previous view's content on Back. |
| `tst_inline_images.qml` | Thumbnail-to-original conversion, SVG/lossy thumbnail handling, unchanged non-thumbnail URLs, full-resolution image links, and captions carried into image links. |
| `tst_images.qml` | Fixture article/gallery loading, SVG preview selection with original fallback, unchanged raster URLs, URL/caption in `ImageSelectionState`, and opening/closing the separate fullscreen window (Close, Escape, and mouse Back) without changing the main window or navigation stack. See the regression notes below for mouse Back and image request counts. |
| `tst_history.qml` | Article visits through `ArticleState`, newest-first order, revisiting an article without duplicating its history entry, visible but disabled unknown-edition entries, failed-clear feedback with retained entries and enabled Clear, errors predating view creation, and unavailable-state text. |
| `tst_navigation.qml` | Real Main window: Search/History push, toolbar and mouse Back pop, Home returns to the original root and is idempotent. Language changes retain that root, clear home news, and request/refill data for the selected edition. Featured article navigation and Back, edition-restoring history navigation, cached history selection, duplicate-history-view prevention, and rendering recent most-read fallback content are covered. |
| `tst_home.qml` | Featured/news/on-this-day/did-you-know pending, ready, empty, error, and retry states; partial failures; duplicate-retry suppression; delayed refresh and language-transition rejection across the Did-you-know summary stage; featured thumbnails, image failure independence, article navigation, URL encoding, narrow/normal viewport width, and article/history isolation. |
| `tst_sections.qml` | Deferred sections, click-to-heading scrolling, viewport tracking/highlight, discarding pending success/error feedback when the article clears, preserving reading position on panel resize, empty results, and request errors. |
| `tst_feature_isolation.qml` | Search success/empty/error responses do not complete a pending article or clear its errors; section/gallery failures do not affect article feedback. Language transitions clear search, sections, gallery, and image selection and reject an obsolete gallery reply. |
| `tst_scrollbars.qml` | Article scrollbar placement and interactivity across widths, drag scrolling, and mirrored left-to-right/right-to-left layouts. |

**`tst_images.qml` regression notes:** Mouse Back closes the fullscreen viewer
after mouse release handling finishes so the window does not retain the mouse
grab and disrupt gallery hover. The tests verify that hover still works after
closing and that a pending close callback cannot close a viewer that has since
been reopened. Image request counts allow reloads caused by native fullscreen
resizing; API request counts remain exact to catch duplicate data requests.

## QML test harness and fixtures

`tests/qml/qml_test_main.cpp` uses `QUICK_TEST_MAIN_WITH_SETUP`. In
`applicationAvailable()`, it installs the fake network factory and registers
`HistoryState`, `NavigationState`, `SettingsState`, `ImageSelectionState`,
`ArticleState`, `SearchState`, and `ClipboardHelper` before the engine imports
`wikipedia_qt`. Registering after that import would finalize/protect the module
and reject singleton registration.

The engine setup adds the generated QML import/plugin paths, installs the SVG
provider, and exposes `networkFixtures` and `testSupport`. Both C++ API clients
and QML image loading use the fake transport. Requests match by HTTP method
and normalized URL, including query parameters. Responses come from committed
fixtures in `tests/qml/fixtures/` or bodies registered by individual tests.
Unexpected requests cause an immediate fatal failure; there is no live-network
fallback. Tests also check captured QML warnings.

`NetworkFixtureController` is declared in
`tests/qml/support/FakeNetworkAccessManager.h`. Useful controls include:

- `addFixture(...)` for response bodies, content type, HTTP status, and network errors.
- `requestCount`, `requests`, and `clearRequests()` for the request log.
- `deferNextReply()` and `pendingReplyCount` for deterministic loading checks.
- `completeNextReply()` or `completeReply(url)` to release deferred replies.
- `deferNextReply(true)` to allow a deferred response to finish after abort,
  exercising stale-reply rejection rather than relying on cancellation alone.

Replies normally complete asynchronously. `clearRequests()` clears the request
log and resets the next-reply deferral flags; it does not remove registered
fixtures or complete existing pending replies.

The harness redirects `XDG_DATA_HOME` to a temporary directory before creating
history state, uses dedicated application metadata, and checks that its data
path stays inside that directory. Each QML file runs in a separate process,
with its own singleton instances and database. Cases within a file share that
process, so their setup/cleanup must reset relevant state and finish pending
replies. The harness destroys its state objects before removing the directory.

## Adding tests

Add a C++ test executable and CTest registration in `tests/CMakeLists.txt`.
Link backend tests to `wikipedia_qt_core` and `Qt::Test`; link
`wikipedia_test_network` when fixtures are needed. Use `QTEST_GUILESS_MAIN`
when a `QCoreApplication` is sufficient; GUI-dependent tests need the offscreen
platform on headless runners.

Add QML coverage as `tests/qml/tst_*.qml`. CMake's `CONFIGURE_DEPENDS` glob
registers each file automatically on the next configure/build. Use fixtures
for every request, stable `objectName` selectors, and `tryCompare`/`tryVerify`
for asynchronous behavior. Assert `testSupport.qmlWarningCount` and appropriate
request counts. Prefer deferred replies over timing sleeps for pending states.

## Remaining coverage and limits

- Home behavior has dedicated fake-transport coverage in `tst_home.qml` and
  navigation coverage in `tst_navigation.qml`. The home tests use a missing
  local image path to verify image failure handling; they do not depend on a
  live image host.
- History tests recreate state/database owners within one process and inject SQLite
  trigger failures. They do not launch a separate application process, inject disk-full
  or commit failures, or verify file compaction. Retention bounds rows to 50; it does
  not shrink the SQLite file. Initialization failures require application restart
  after repair; automatic initialization retries and unsaved memory history are
  outside scope.
- Request-isolation tests cover selected stale-reply paths, not every stage of
  the HTML/math-image/metadata chains. They do not test request deadlines.
- Headless fixture tests do not establish live API compatibility or validate
  installed/packaged application deployment.
