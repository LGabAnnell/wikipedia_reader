# AGENTS.md

Guidance for AI agents working in the `wikipedia_reader` repository.

## 1. Project Overview

A Qt6 / QML desktop application that reads Wikipedia content: article search,
full article viewing with HTML rendering, article sections, image galleries,
a home screen (featured article of the day, in-the-news, on-this-day,
did-you-know), and persistent browsing history (SQLite). It targets the
selected Wikipedia edition’s REST/Action APIs.

See `README.md` for setup and `docs/full_testing.md` for tests. The CMake project is named `wikipedia_qt`; the executable
is `appwikipedia_qt`.

## 2. Tech Stack

- **Language:** C++20 (`CMAKE_CXX_STANDARD 20`, required)
- **UI:** QML / Qt Quick (Qt 6.8+, `qt_standard_project_setup(REQUIRES 6.8)`)
- **Build:** CMake >= 3.16
- **Qt modules:** Quick, Core, Network, Sql, Gui, Svg, SvgWidgets, Widgets, Test
- **Third-party:** `Lexbor 3.0+` (HTML5 parsing in `html_processor`) and `tinyxml2` (SVG icon recoloring)
- **Persistence:** SQLite via QtSql (`QStandardPaths::AppDataLocation/history.db`)
- **Networking:** `QNetworkAccessManager` against `en.wikipedia.org` and
  `api.wikimedia.org`

## 3. Directory Structure

```
wikipedia_reader/
├── CMakeLists.txt            # Top-level build config; finds Qt6, Lexbor + tinyxml2
├── src/
│   ├── main.cpp              # Entry point; registers QML singletons
│   ├── Main.qml              # Root ApplicationWindow + StackView
│   ├── SearchScreen.qml      # Search view: controls, loading/empty/error feedback, Sidebar
│   ├── constants.{h,cpp}     # QML_SINGLETON view-name constants
│   ├── wikipedia_models.h    # Q_GADGET data structs (search_result, page, ...)
│   ├── wikipedia_search_client.{h,cpp}   # Action API search
│   ├── wikipedia_page_client.{h,cpp}      # Action API pages/sections/images
│   ├── wikipedia_featured_client.{h,cpp} # Wikimedia feed: featured article
│   ├── wikipedia_home_client.{h,cpp}     # Wikimedia feed: news/on-this-day/dyk
│   ├── html_processor.{h,cpp}            # Lexbor-based HTML cleanup
│   ├── wikipedia_network_access_manager.h  # Injectable QNetworkAccessManager factory
│   ├── CMakeLists.txt        # Main `wikipedia_qt` QML module (URI wikipedia_qt)
│   ├── state/
│   │   ├── ArticleState.{h,cpp}    # Article loading, cache, feedback
│   │   ├── SearchState.{h,cpp}     # Search lifecycle and results
│   │   ├── SettingsState.{h,cpp}   # Wikipedia edition and transition signals
│   │   ├── ImageSelectionState.{h,cpp} # Fullscreen image selection
│   │   ├── ClipboardHelper.{h,cpp} # Clipboard access
│   │   ├── HistoryState.{h,cpp}   # History singleton (in-memory + DB)
│   │   ├── NavigationState.{h,cpp}# StackView navigation + back-button event filter
│   │   └── db/
│   │       └── HistoryDatabase.{h,cpp}  # SQLite history persistence
│   └── modules/              # Each module = qt_add_qml_module library
│       ├── SearchBarModule/      # URI wikipedia_qt.SearchBar
│       ├── SidebarModule/       # URI wikipedia_qt.Sidebar
│       ├── HistoryModule/       # URI wikipedia_qt.History
│       ├── HeaderModule/        # URI wikipedia_qt.Header (+ SvgImageProvider)
│       ├── HomeModule/          # URI wikipedia_qt.Home (HomeScreen + sections)
│       ├── ContentDisplayModule/ # URI wikipedia_qt.ContentDisplay
│       ├── ImageDisplayModule/  # URI wikipedia_qt.ImageDisplay
│       └── SectionModule/       # URI wikipedia_qt.Section
├── tests/                    # Qt Test / CTest tests
│   └── qml/                  # Qt Quick Test harness (fake network transport)
│       ├── support/          # FakeNetworkAccessManager, FakeNetworkReply, QmlTestSupport
│       ├── fixtures/         # Committed JSON/HTML/PNG fixtures
│       ├── tst_*.qml         # Test files (auto-globbed into CTest as QmlTests.<name>)
│       └── qml_test_main.cpp # QUICK_TEST_MAIN_WITH_SETUP entry
├── examples/                 # Usage example for WikipediaClient
├── icons/                    # SVG icons (home, history, back, search)
├── styles/                   # CSS (table_style.css)
├── scripts/run_debug.sh      # Build + run in debug mode
├── test.sh                   # Legacy destructive rebuild + tests
└── .vibe/                    # Vibe agent config (cpp agent + prompt)
```

### Key entry points
- `src/main.cpp` — creates `QGuiApplication`, registers singletons
  (`ArticleState`, `SearchState`, `SettingsState`, `ImageSelectionState`,
  `ClipboardHelper`, `HistoryState`, `NavigationState`) via
  `qmlRegisterSingletonInstance("wikipedia_qt", 1, 0, ...)`, installs the
  `svg` image provider, loads QML via `engine.loadFromModule("wikipedia_qt", "Main")`.
- `src/Main.qml` — `ApplicationWindow` with a `Header` and a `StackView`
  hosting `searchView`, `articleComponent`, `homeComponent`, `historyComponent`,
  `imageGalleryComponent`. Wires `NavigationState` to the stack on `onCompleted`.

## 4. Build / Run / Test

```bash
# Configure (debug)
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build -j14

# Run (binary output dir is bin/)
./build/bin/appwikipedia_qt

# Configure with tests
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build -j14

# Run tests via CTest
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure

# Run only the Qt Quick Tests
ctest --test-dir build -R QmlTests

# Debug run helper (rebuild from scratch with `rebuild` arg)
./scripts/run_debug.sh
./scripts/run_debug.sh rebuild
```

`BUILD_TESTING` is an `option(... OFF)` in the top-level `CMakeLists.txt`;
when ON it `add_subdirectory(tests)`.

QML debug logging: build with `-DCMAKE_BUILD_TYPE=Debug` (defines `DEBUG`,
enables `QtDebugMsg`) and/or pass `-DQT_QML_DEBUG=ON`. `run_debug.sh` also sets
`QT_LOGGING_RULES="default.debug=true"`.

## 5. Architecture & Key Modules

### State singletons (registered to QML)

- `SettingsState` owns the selected language. It emits `languageTransitionStarted`
  to invalidate edition-bound state before `languageChanged` triggers refreshes.
- `ArticleState` owns article loading, feedback, and the article cache. History
  selection restores the saved edition before loading the page.
- `SearchState` owns search text, results, busy/completion state, and errors.
  `SearchBarModel` adapts it to the input controls.
- `ImageSelectionState` owns the selected image URL and caption; language
  transitions clear the selection. `ClipboardHelper` provides clipboard access.
- `HistoryState` keeps up to 50 in-memory entries backed by `HistoryDatabase`.
  Identity is `(language, pageId)`; legacy unknown-edition entries cannot open.
  Database retention and write-failure consistency remain known issues.
- `NavigationState` registers named views, manages StackView navigation, and
  filters the mouse Back button. `Constants` provides view-name strings.

Backend implementation, meta-objects, and singleton storage live in the shared
`wikipedia_qt_core` library. `wikipedia_runtime` contains navigation separately.

### Wikipedia API clients (`src/wikipedia_*_client.{h,cpp}`)
All use `QNetworkAccessManager` and signal/slot async patterns. URLs below show
the English defaults; clients capture the selected edition and reject obsolete
request generations.
- `WikipediaSearchClient` — `search(query, limit)` -> `searchCompleted(QVector<search_result>)`. Base URL `https://en.wikipedia.org/w/api.php`.
- `WikipediaPageClient` — `getPage(title)`, `getPageById(pageid)`,
  `getPageWithImages(pageid)`, `resolveTitleToPageId(title)`,
  `getSections(title)` -> `pageReceived`, `pageWithImagesReceived`,
  `pageIdResolved`, `sectionsReceived`.
- `WikipediaFeaturedClient` — `getFeaturedArticleOfTheDay()` against
  `https://api.wikimedia.org/feed/v1/wikipedia/en/featured/<date>`.
- `WikipediaHomeClient` — `getNewsItems()`, `getOnThisDayEvents(m,d)`,
  `getDidYouKnowItems()` against the Wikimedia feed + REST APIs.

### Data models (`src/wikipedia_models.h`)
`Q_GADGET` + `QML_ELEMENT` structs exposed to QML: `search_result`, `page`,
`featured_article`, `news_item`, `on_this_day_event`, `did_you_know_item`,
`history_item`, `section`. Use `Q_PROPERTY(... MEMBER ...)`.

### Feature modules (`src/modules/<Module>/`)
Each module is a `qt_add_qml_module` library with URI `wikipedia_qt.<Module>`,
containing QML view file(s) + a `*Model.{h,cpp}` C++ backend
(`Q_OBJECT` + `QML_ELEMENT`). Models hold state and call the API clients /
state singletons. The main `wikipedia_qt` module (`src/CMakeLists.txt`)
depends on all submodules.

### Search flow
- **`SearchBarModel`** trims submitted text and ignores empty or whitespace-only
  queries. It exposes `isSearching` and read-only `hasCompletedSearch` to QML;
  a new request clears completion state, prior results, and errors.
- **`SearchBar`** disables its text field and button while searching, and keeps
  the button disabled for blank or whitespace-only input. Its stable QML test
  selectors are `searchInput` and `searchButton`.
- **`SearchScreen`** displays `searchLoadingIndicator` during a pending search,
  `searchStatusLabel` for errors or a successful empty response, and passes
  results to the `searchResults` sidebar. The sidebar list and delegates expose
  `searchResultsList` and `searchResultContent-<index>` selectors respectively.
- Search completion and failure clear `SearchState.isSearching`. “No results found.” is only shown after a successful response,
  never before the first search or while a retry is pending.

### Navigation
StackView-based. `Main.qml` registers `Component`s with `NavigationState`
on startup. Use `StackView.push()`/`pop()` — NOT `replace` — to preserve
history (see `.continue/rules/qt-stackview-navigation*.md`). The "Home"
action pops to root rather than pushing a new root.

## 6. Conventions

### C++ / Qt
- Modern C++ (C++20), Qt best practices, RAII. Parent-child QObject ownership
  for memory management (`new WikipediaSearchClient(this)`).
- `Q_OBJECT` + `QML_ELEMENT` for QML-exposed classes; `Q_GADGET` for value
  structs; `QML_SINGLETON` for `Constants`; `QML_UNCREATABLE("Singleton")` for
  state singletons registered via `qmlRegisterSingletonInstance`.
- Async via signal/slot; errors via `qWarning()` + `errorOccurred` signals;
  user-facing strings via `tr()`.
- Doxygen-style `@brief`/`@param` comments for public APIs.
- `.editorconfig` defines C++ formatting: braces on same line, pointer/
  reference aligned right, no space before function-call parens, space after
  control-flow keywords, etc. Match it.
- Ignore the `build/` directory when scanning files.

### QML
- Use `console.warn` (NOT `console.log`) for debug statements in `.qml` files
  (per `.continue/rules/c++.md`).
- Import modules by URI (`import wikipedia_qt`, `import wikipedia_qt.History`,
  etc.). Versioned imports (`import wikipedia_qt 1.0`) appear in older files.
- Let the parent `StackView` manage view sizing; avoid `anchors.fill: parent`
  on stacked items (see `History.qml` comment).

### Database (SQLite)
Per `.continue/rules/qt-sqlite-best-practices.md` and
`qt-database-error-handling.md`:
- Use a named connection (`"history_db"`); open once, reuse.
- Store DB in `QStandardPaths::AppDataLocation`.
- Prepared statements with bind values; transactions for batch ops.
- `executeQuery` helper for consistent error handling; emit `databaseError`
  with the SQL text; `tr()` for messages; rollback on failure.
- Close + remove connection in destructor (guard with
  `QCoreApplication::instanceExists()`).

### Event filters
Non-blocking: only intercept needed events (e.g. `Qt::BackButton`), return
`false` for all others (`.continue/rules/qt-event-filter-best-practices.md`).

## 7. Testing

Use [docs/full_testing.md](docs/full_testing.md) as the authoritative guide for
build dependencies, test inventory, fixtures, coverage limits, and adding tests.

- Run `./scripts/run_tests.sh` for an incremental build and offscreen CTest run.
  It supports `-R <regex>` and opt-in live testing with `-n`.
- The legacy `test.sh` deletes `build/` on every run; prefer the incremental runner.
- Backend tests link `wikipedia_qt_core`; fixture tests also link
  `wikipedia_test_network`. Use `QTEST_GUILESS_MAIN` when a core application
  suffices. Existing GUI-backed tests need `QT_QPA_PLATFORM=offscreen` headlessly.
- QML files in `tests/qml/tst_*.qml` are registered individually with CTest.
  The harness registers real singletons before importing `wikipedia_qt`, installs
  fake transport for API and QML image requests, and isolates history in a
  temporary data directory. Unexpected requests fail immediately.
- Prefer deferred fixture replies and asynchronous assertions for pending states.
  Check QML warnings and request counts. See the guide for transport controls.

## 8. Git Workflow

- **Main branch:** `master`.
- **Branch naming:** short descriptive kebab-case, often prefixed by intent:
  `add_search_in_article`, `add-sections`, `sections-search`,
  `refacto-and-fix`, `section-scroll-fix`.
- **Commit messages:** descriptive subject line with an optional parenthesized
  type prefix: `(feat):`, `(bugfix):`, `(chore):`. Some older commits use plain
  descriptive subjects without a prefix.
  - Do NOT wrap class/file/module names in backticks or quotes in commit
    messages (per
    `.continue/rules/commit-message-formatting-for-class-names.md`).
- **PRs:** GitHub PRs merged into `master` (e.g. PR #3, #4, #5). Feature
  branches are merged.

## 9. Existing Guides / Agent Config

- No `CONTRIBUTING.md` exists.
- `docs/README.md` indexes current guides and archived designs.
- `docs/full_testing.md` is the authoritative testing guide.
- `.continue/rules/` — Continue IDE rule files (most are `alwaysApply: true`):
  `c++.md`, `qt-database-error-handling.md`, `qt-event-filter-best-practices.md`,
  `qt-sqlite-best-practices.md`, `qt-stackview-navigation.md`,
  `qt-stackview-navigation-standards.md`,
  `commit-message-formatting-for-class-names.md`.
- `.vibe/agents/cpp.toml` + `.vibe/prompts/cpp.md` — Vibe "cpp" subagent with
  `web_search` tool; prompt mirrors the C++/Qt guidelines above.

## 10. Configuration

- No `.env` / `.env.example`. No runtime config file.
- API URLs use the selected language; English examples:
  - `https://en.wikipedia.org/w/api.php` (search, page)
  - `https://api.wikimedia.org/feed/v1/wikipedia/en/featured/<date>` (featured)
  - `https://en.wikipedia.org/api/rest_v1/...` (home/random/summary)
- Build-time options (CMake):
  - `BUILD_TESTING` (OFF) — build `tests/`.
  - `CMAKE_BUILD_TYPE=Debug` — defines `DEBUG` macro, enables debug logging.
  - `QT_QML_DEBUG` — defines `QT_QML_DEBUG` for the QML debugger.
- `QT_QML_GENERATE_QMLLS_INI` is ON (`.qmlls.ini`/`qmldir` are gitignored as
  generated).
- History DB path: `QStandardPaths::AppDataLocation` + `/history.db`.
