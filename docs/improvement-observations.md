# Improvement Observations

Review of the `wikipedia_reader` repository (2026-09-29). The codebase is
well organized overall (clean state/module split, solid Qt Quick test
harness); the items below focus on hardening and structure.

## Highest value

### No CI
There is no `.github/workflows` despite a solid CTest suite in `tests/` and
`tests/qml/`. A Linux runner with Qt 6.8 that configures with
`-DBUILD_TESTING=ON` and runs `ctest --test-dir build` (including the
`QmlTests.*` suite) would protect the QML and client tests from regression.

### No network timeouts
None of the `Wikipedia*Client` classes (`src/wikipedia_search_client.cpp`,
`src/wikipedia_page_client.cpp`, `src/wikipedia_featured_client.cpp`,
`src/wikipedia_home_client.cpp`) set a transfer timeout or retry. A stalled
request leaves `GlobalState.isLoading` stuck in the loading state and the
UI blocked. `QNetworkAccessManager::setTransferTimeout` (available in Qt 6.8)
or a watchdog timer per request would fix this cheaply. Consider a bounded
retry with backoff for transient failures.

### Duplicate source compilation in tests
Each test target in `tests/CMakeLists.txt` re-compiles the same set of
`src/*.cpp` files (the four API clients, `src/state/GlobalState.cpp`,
`src/state/HistoryState.cpp`, `src/state/db/HistoryDatabase.cpp`,
`src/html_processor.cpp`). Extracting a small `wikipedia_qt_core` static
library that both the app and the tests link against removes duplication and
the risk of test builds drifting from the app build.

## Structural

### GlobalState is drifting into a god object
`src/state/GlobalState.h` (~129 lines) owns page state, image gallery state
(`currentImageUrl`, `currentImageDescription`), section tracking
(`currentSectionIndex`), language, and the article cache. Splitting
gallery/section concerns into their own state objects (or reusing the module
models under `src/modules/` that already exist) would keep the state layer
maintainable. The static `QPointer<GlobalState> m_instance` singleton in
`src/state/GlobalState.cpp` is a secondary smell in the same direction.

### HTML/image handling entangled in the page client
`src/wikipedia_page_client.cpp` contains SVG rasterization (`rasterizeSvg`),
`stripHtml`, and a hand-rolled HTML entity decoder that only handles six
entities (`&amp;`, `&lt;`, `&gt;`, `&quot;`, `&#39;`, `&nbsp;`).
`QTextDocumentFragment::fromHtml` or a proper unescaping pass would be more
correct for arbitrary Wikipedia markup. The SVG rasterization pipeline also
belongs with `src/html_processor.cpp` or a dedicated image utility rather
than the page client.

### Documentation split and missing README
`TESTING.md` and `docs/full_testing.md` cover the same area with diverging
content — consolidate into one document. AGENTS.md notes that no `README.md`
exists; a basic README (overview, build, run, test instructions) would close
that gap.

## Small wins

### QML test gap on the home screen
`tests/qml/` covers search, article, sections, images, history, and
navigation, but not `HomeScreen` (featured article of the day, on-this-day,
did-you-know). A `tst_home.qml` would close the gap; the fake-network fixture
harness (`tests/qml/support/`, `tests/qml/fixtures/`) already supports it.

### Repo clutter
Stray build directories (`Testing/Temporary`, `build_htmlprocessor_tests`,
`debug/`, `release/`) are mostly gitignored, but a `clean.sh` next to
`scripts/run_debug.sh` would keep the tree tidy.

### Stale example
`examples/wikipedia_client_usage_example.cpp` is not referenced by the build
and lags the current client API. Refresh or remove it.

### Unpinned tinyxml2
`find_package(tinyxml2 REQUIRED)` in `CMakeLists.txt` has no version. Pinning
a minimum version makes upstream breaking changes fail loudly instead of
silently compiling against an incompatible API.
