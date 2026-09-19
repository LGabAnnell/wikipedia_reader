# Testing

This document describes the test suite of the `wikipedia_qt` application: how to
build and run it, what each test covers, and how to add new tests.

## Overview

The suite has four layers:

1. **C++ unit tests** (`tests/*.cpp`) — Qt Test based tests for the state
   singletons, the SQLite history database, the HTML processor, and the
   `Wikipedia*Client` API wrappers.
2. **QML layout test** — `test_sidebar_layout` instantiates the `Sidebar` QML
   component headlessly and checks its layout.
3. **Opt-in network integration test** — `test_search_display` exercises the
   real search flow against `en.wikipedia.org`. It is disabled unless
   `RUN_NETWORK_TESTS=1` is set in the environment.
4. **Qt Quick Test harness** (`tests/qml/`) — a `QUICK_TEST_MAIN_WITH_SETUP`
   runner that loads real QML components from the application modules with all
   network traffic replaced by a fake transport serving local fixtures.

All tests run headlessly (`QT_QPA_PLATFORM=offscreen`); no display is needed.

## Quick start

The easiest way to run everything:

```bash
./scripts/run_tests.sh
```

The script configures `build/` with tests enabled (only if not already
configured), builds incrementally, and runs CTest. Useful variants:

```bash
./scripts/run_tests.sh -r             # wipe build/ and rebuild from scratch
./scripts/run_tests.sh -R QmlTests    # only run tests matching a ctest regex
./scripts/run_tests.sh -n             # also enable live-network tests
```

Equivalent raw commands:

```bash
# Configure with tests (BUILD_TESTING is OFF by default)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON

# Build
cmake --build build -j14

# Run all tests
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure

# Run a subset
QT_QPA_PLATFORM=offscreen ctest --test-dir build -R GlobalStateTest
```

The legacy `./test.sh` at the repo root wipes `build/` and rebuilds from
scratch on every run; prefer `scripts/run_tests.sh`.

## Build configuration

Tests are opt-in at configure time:

```cmake
option(BUILD_TESTING "Build tests" OFF)
```

With `-DBUILD_TESTING=ON` the top-level `CMakeLists.txt` additionally:

- finds the Qt6 `Test` and `QuickTest` components,
- adds the `tests/` subdirectory, which registers every test with CTest.

Without the flag, no test targets are built and `ctest` reports no tests.

## Test inventory

| CTest name | Kind | Covers | Network | Notes |
|---|---|---|---|---|
| `GlobalStateTest` | Unit | `GlobalState` state transitions, article cache, client wiring | Offline | Compiles `GlobalState`, `HistoryState`, `HistoryDatabase` and all four clients directly |
| `test_search_display` | Integration | Live search flow through `GlobalState` + `SearchBarModel` | **Live** unless `RUN_NETWORK_TESTS` unset | Exits 0 immediately when `RUN_NETWORK_TESTS` is not `1` |
| `test_sidebar_layout` | Layout | `Sidebar` QML component layout | Offline | Runs with working directory `src/modules`; CTest sets `QT_QPA_PLATFORM=offscreen` and `QML_IMPORT_PATH` |
| `HtmlProcessorTest` | Unit | `html_processor` HTML cleanup (tinyxml2) | Offline | |
| `HistoryDatabaseTest` | Unit | `HistoryDatabase` SQLite persistence | Offline | Isolates its database by setting the application name to `HistoryDatabaseTest` (DB lives under `QStandardPaths::AppDataLocation`) |
| `WikipediaSearchClientTest` | Unit | `WikipediaSearchClient` request/response handling | Offline | |
| `WikipediaPageClientTest` | Unit | `WikipediaPageClient` pages/sections/images | Offline | |
| `WikipediaFeaturedClientTest` | Unit | `WikipediaFeaturedClient` featured-article feed parsing | Offline | |
| `WikipediaHomeClientTest` | Unit | `WikipediaHomeClient` news/on-this-day/did-you-know parsing | Offline | |
| `QmlTests.tst_smoke` | QML | State singletons resolve and start clean; `Sidebar` instantiates without warnings | Offline (fake transport) | One CTest entry per `tests/qml/tst_*.qml` file, each in its own process |

## Qt Quick Test harness (`tests/qml/`)

The `QmlTests` executable is built from `tests/qml/qml_test_main.cpp` plus the
support classes in `tests/qml/support/`. It uses
`QUICK_TEST_MAIN_WITH_SETUP(wikipedia_qt, QmlTestSetup)`.

### Singleton registration order

The setup object registers the `GlobalState`, `HistoryState` and
`NavigationState` singleton instances in `applicationAvailable()`, **before**
any QML engine imports the `wikipedia_qt` module. Importing the module first
would finalize and protect it, rejecting these registrations. Keep that order
if you touch the setup.

### Fake network transport

- `WikipediaNetwork::installNetworkAccessManagerFactory` installs a
  `NetworkFixtureController` as the application-wide `QNetworkAccessManager`
  factory, and the test engine also uses it via
  `QQmlEngine::setNetworkAccessManagerFactory`. No QML test ever reaches the
  real network.
- `NetworkFixtureController` (exposed to QML as the `networkFixtures` context
  property) serves canned replies keyed by method + URL:
  - `addFixture(method, url, body, contentType, statusCode, networkError, errorString)` —
    register a reply; `statusCode` defaults to 200, `networkError` to
    `QNetworkReply::NoError`, so HTTP errors and network failures can be
    simulated.
  - `requestCount` / `requests` / `clearRequests()` — inspect and reset the
    recorded request log.
  - Any request without a matching fixture emits `unexpectedRequest`, which the
    setup turns into `qFatal` — unexpected requests are fatal by design.
- `QmlTestSupport` (exposed as the `testSupport` context property) counts QML
  warnings (`qmlWarningCount`, `qmlWarnings`, `clearQmlWarnings()`) and mirrors
  the request counters, so tests can assert "no warnings, no stray requests".
- The setup points `XDG_DATA_HOME` at a `QTemporaryDir` and sets the
  application name to `QmlTests` (organization `WikipediaReaderTests`), so the
  history database is created in an isolated, throwaway location. A guard
  `qFatal`s if the app data path escapes the temporary directory.
- The `svg` image provider from `HeaderModule` is installed on the engine so
  QML that references `image://svg/...` icons resolves.

### Fixtures

`tests/qml/fixtures/` holds canned payloads (`search-results.json`,
`article.html`, `fixture.png`). The fixture directory is baked into the
`QmlTests` binary via the `QML_TEST_FIXTURE_DIR` compile definition, and the
`NetworkFixtureController` constructor pre-registers three fixture replies
from it (a search API response, a REST article HTML page, and a PNG image).
Tests can add more replies at runtime with `addFixture`.

### Per-file test processes

`tests/qml/CMakeLists.txt` globs `tst_*.qml` and registers one CTest entry per
file (`QmlTests.<name>`), running `QmlTests -input <file>` with
`QT_QPA_PLATFORM=offscreen` and `QML_IMPORT_PATH` pointing at the build tree.
Each QML test file therefore runs in its own process with a fresh setup.

## Network tests

- `test_search_display` is the only test that talks to the live
  `en.wikipedia.org` API, and only when `RUN_NETWORK_TESTS=1` is exported
  (e.g. `./scripts/run_tests.sh -n`). Without the variable it prints a notice
  and exits 0, so offline runs stay green.
- The QML tests are fully offline by design: the fake transport serves every
  reply, and any request without a fixture aborts the test. They can be used to
  prove network isolation, e.g. by running them inside a network namespace:

  ```bash
  unshare -rn ./scripts/run_tests.sh -R QmlTests
  ```

## Writing new tests

### C++ Qt Test target

1. Add `tests/MyThingTest.cpp` using `QTEST_MAIN` / `QSignalSpy`.
2. In `tests/CMakeLists.txt`, add a `qt_add_executable` that compiles your test
   plus the sources under test directly (existing tests do not link the app
   target), link `Qt::Test` and the needed Qt modules, and
   `add_test(NAME MyThingTest COMMAND MyThingTest)`.
3. If the test instantiates QML, set the CTest `ENVIRONMENT` property to
   `QT_QPA_PLATFORM=offscreen` (see `test_sidebar_layout`).

### QML test file

1. Drop a new `tests/qml/tst_*.qml` file into `tests/qml/`. The glob in
   `tests/qml/CMakeLists.txt` auto-registers it as `QmlTests.<name>` — no CMake
   edits needed (re-run the script so the glob re-triggers).
2. In the file, `import QtTest` and write `TestCase` blocks. The context
   properties `testSupport` and `networkFixtures` are available without
   imports.
3. Register any reply your component will request:

   ```qml
   networkFixtures.addFixture("GET", "https://en.wikipedia.org/w/api.php?...",
                              JSON.stringify({ /* body */ }))
   ```

   and assert cleanliness at the end:

   ```qml
   compare(testSupport.requestCount, 0)      // or the expected count
   compare(testSupport.qmlWarningCount, 0, testSupport.qmlWarnings.join("\n"))
   ```

## Headless / CI notes

- All GUI-touching tests run with `QT_QPA_PLATFORM=offscreen`; no X11/Wayland
  display is required. `scripts/run_tests.sh` exports it for the whole CTest
  run as a safety net.
- The QML tests are deterministic and offline (fake transport, temp
  `XDG_DATA_HOME`), which makes them CI-friendly; the network namespace trick
  above can be used to verify no test leaks to the real network.
- Only `test_search_display` needs internet, and only when explicitly enabled.

## Troubleshooting

- **"module is protected" / singleton registration `qFatal` in QML tests** —
  the singleton registration in `QmlTestSetup::applicationAvailable()` must
  happen before any engine imports `wikipedia_qt`. If you add an import or
  engine creation earlier in the setup, move it after the registrations.
- **`qt.qpa.plugin: Could not load the Qt platform plugin "offscreen"`** — the
  Qt `offscreen` platform plugin is missing from the installation or
  `QT_PLUGIN_PATH`. Install the full Qt 6.8+ runtime.
- **Stale build directory** — if test targets are missing or behave oddly
  (e.g. the build was configured without `BUILD_TESTING`), run
  `./scripts/run_tests.sh -r` to wipe and reconfigure, or check that
  `build/CMakeCache.txt` contains `BUILD_TESTING:BOOL=ON`.
- **QML test cannot find `wikipedia_qt` or a submodule** — the harness adds the
  build tree to the engine import/plugin paths (`QML_TEST_IMPORT_PATH`). Make
  sure the build is up to date so the QML modules are compiled into
  `build/wikipedia_qt/`.
