# Proposed fix for P2: preserve GUI-less backend test startup

Status: proposal only; the implementation has not been changed or validated.

## Problem

`wikipedia_qt_core` publicly links `Qt::Gui` in the top-level
`CMakeLists.txt`. Consumers inherit `QT_GUI_LIB`, including backend tests
that previously linked only non-GUI Qt modules. Their existing `QTEST_MAIN`
entry points consequently construct `QGuiApplication` instead of
`QCoreApplication`. Without a display or an explicitly selected offscreen
platform, GUI initialization aborts before any test executes.

The review confirmed this regression for `HistoryDatabaseTest` and
`WikipediaSearchClientTest`. `WikipediaFeaturedClientTest` and
`WikipediaHomeClientTest` have the same entry-point and dependency pattern.
The installed Qt Test header also confirms that `QTEST_MAIN` selects the
application type from `QT_WIDGETS_LIB` and `QT_GUI_LIB`, while
`QTEST_GUILESS_MAIN` explicitly constructs `QCoreApplication`.

## Recommended change

Make the application requirement explicit in the four affected test files:

| File | Existing entry point | Proposed entry point |
| --- | --- | --- |
| `tests/HistoryDatabaseTest.cpp` | `QTEST_MAIN(HistoryDatabaseTest)` | `QTEST_GUILESS_MAIN(HistoryDatabaseTest)` |
| `tests/WikipediaSearchClientTest.cpp` | `QTEST_MAIN(WikipediaSearchClientTest)` | `QTEST_GUILESS_MAIN(WikipediaSearchClientTest)` |
| `tests/WikipediaFeaturedClientTest.cpp` | `QTEST_MAIN(WikipediaFeaturedClientTest)` | `QTEST_GUILESS_MAIN(WikipediaFeaturedClientTest)` |
| `tests/WikipediaHomeClientTest.cpp` | `QTEST_MAIN(WikipediaHomeClientTest)` | `QTEST_GUILESS_MAIN(WikipediaHomeClientTest)` |

For example:

```diff
-QTEST_MAIN(HistoryDatabaseTest)
+QTEST_GUILESS_MAIN(HistoryDatabaseTest)
```

These tests cover SQLite persistence and JSON parsing; their current test
bodies do not require windows, clipboard access, or GUI rendering. A
`QCoreApplication` preserves application metadata, paths, and event-loop
support without initializing a platform plugin. Keep their includes and moc
includes as they are. No CMake change is needed for this approach.

Use `QTEST_GUILESS_MAIN` rather than `QTEST_APPLESS_MAIN`: the database test
uses application metadata and `QStandardPaths`, so retaining the core
application is appropriate.

Keep the existing GUI setup for Qt Quick tests and `test_sidebar_layout`,
including their CTest offscreen environment. `GlobalStateTest`,
`HtmlProcessorTest`, and `WikipediaPageClientTest` already linked `Qt::Gui`
before this patch; changing their application type is outside this regression
fix and would require checking their GUI-dependent code paths separately.

Retain the core's current dependency declarations for this focused fix.
Merely making `Qt::Gui` private is not a reliable substitute: public headers
currently include GUI headers, and static-library link requirements may
still reach consumers. Explicit test entry points remain stable when the
core is made shared as proposed for P1.

## Alternative

The review also permits setting `QT_QPA_PLATFORM=offscreen` in the affected
tests' CTest environment. That would allow `QGuiApplication` to start on a
headless runner, but direct execution would still require the environment
setting and a working GUI platform plugin. Prefer the explicit GUI-less
entry points to preserve the previous startup requirements.

## Validation and acceptance criteria

Configure and build in a fresh directory:

```bash
cmake -S . -B build-p2 -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build-p2 -j14
mkdir -p build-p2/test-data
```

Run the four affected tests with display and platform overrides removed:

```bash
env -u DISPLAY -u WAYLAND_DISPLAY -u QT_QPA_PLATFORM \
    XDG_DATA_HOME="$PWD/build-p2/test-data" \
    ctest --test-dir build-p2 --output-on-failure \
    -R '^(HistoryDatabaseTest|Wikipedia(Search|Featured|Home)ClientTest)$'
```

The writable application-data directory isolates the database from the
user's normal data and avoids unrelated filesystem failures.

1. Require all four tests to execute and pass without display connection or
   platform-plugin initialization errors.
2. Obtain executable paths using `ctest --test-dir build-p2 -N -V` and run
   each affected executable directly with the same environment. Require
   success without relying on CTest-specific offscreen settings.
3. Repeat the focused run with `QT_QPA_PLATFORM` set to a deliberately
   nonexistent platform name. It should still pass because these entry
   points construct `QCoreApplication` and do not load a GUI platform plugin.
4. Run the full existing suite with writable test data and
   `QT_QPA_PLATFORM=offscreen` for tests that require a GUI. Require no new
   failures. This broader check supplements the focused run; an all-offscreen
   run alone would conceal the original P2 regression.

The P1 shutdown use-after-free remains a separate issue, covered by
[the shared-core proposal](p1-shared-core-fix-proposal.md).
