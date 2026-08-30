# AGENTS.md

Guidance for AI agents working in the `wikipedia_reader` repository.

## 1. Project Overview

A Qt6 / QML desktop application that reads Wikipedia content: article search,
full article viewing with HTML rendering, article sections, image galleries,
a home screen (featured article of the day, in-the-news, on-this-day,
did-you-know), and persistent browsing history (SQLite). It targets the
English Wikipedia REST/Action APIs.

No README exists. The CMake project is named `wikipedia_qt`; the executable
is `appwikipedia_qt`.

## 2. Tech Stack

- **Language:** C++20 (`CMAKE_CXX_STANDARD 20`, required)
- **UI:** QML / Qt Quick (Qt 6.8+, `qt_standard_project_setup(REQUIRES 6.8)`)
- **Build:** CMake >= 3.16
- **Qt modules:** Quick, Core, Network, Sql, Gui, Svg, SvgWidgets, Widgets, Test
- **Third-party:** `tinyxml2` (HTML parsing in `html_processor`)
- **Persistence:** SQLite via QtSql (`QStandardPaths::AppDataLocation/history.db`)
- **Networking:** `QNetworkAccessManager` against `en.wikipedia.org` and
  `api.wikimedia.org`

## 3. Directory Structure

```
wikipedia_reader/
├── CMakeLists.txt            # Top-level build config; finds Qt6 + tinyxml2
├── src/
│   ├── main.cpp              # Entry point; registers QML singletons
│   ├── Main.qml              # Root ApplicationWindow + StackView
│   ├── SearchScreen.qml      # Search view (SearchBar + Sidebar)
│   ├── constants.{h,cpp}     # QML_SINGLETON view-name constants
│   ├── wikipedia_models.h    # Q_GADGET data structs (search_result, page, ...)
│   ├── wikipedia_search_client.{h,cpp}   # Action API search
│   ├── wikipedia_page_client.{h,cpp}      # Action API pages/sections/images
│   ├── wikipedia_featured_client.{h,cpp} # Wikimedia feed: featured article
│   ├── wikipedia_home_client.{h,cpp}     # Wikimedia feed: news/on-this-day/dyk
│   ├── html_processor.{h,cpp}            # tinyxml2-based HTML cleanup
│   ├── CMakeLists.txt        # Main `wikipedia_qt` QML module (URI wikipedia_qt)
│   ├── state/
│   │   ├── GlobalState.{h,cpp}     # App-wide singleton: search/page/loading state
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
├── examples/                 # Usage example for WikipediaClient
├── icons/                    # SVG icons (home, history, back, search)
├── styles/                   # CSS (table_style.css)
├── scripts/run_debug.sh      # Build + run in debug mode
├── test.sh                   # `ctest --test-dir build/tests`
├── .editorconfig             # C++ formatting rules
├── .continue/rules/          # Continue IDE rule files (Qt/C++ conventions)
└── .vibe/                    # Vibe agent config (cpp agent + prompt)
```

### Key entry points
- `src/main.cpp` — creates `QGuiApplication`, registers singletons
  (`GlobalState`, `HistoryState`, `NavigationState`) via
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
cmake --build build

# Run (binary output dir is bin/)
./build/bin/appwikipedia_qt

# Configure with tests
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build

# Run tests via CTest
ctest --test-dir build            # or: ./test.sh  (runs ctest --test-dir build/tests)

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
```
QML (Main.qml, modules)  --properties/slots-->  GlobalState / HistoryState / NavigationState
        ^                                              |
        | signals (xxxChanged)                          v
        └── QML bindings auto-update        Wikipedia*Client (QNetworkAccessManager)
                                                   -> Wikipedia REST/Action APIs
```

- **`GlobalState`** (`src/state/GlobalState.h`): central app state. Owns the
  four `Wikipedia*Client` instances. Exposes `searchResults`, current page
  (title/extract/id), `isLoading`, `currentPageSections`, `errorMessage`.
  `Q_INVOKABLE loadArticleByPageId(int)`, `loadArticleByTitle(QString)`,
  `fetchSectionsForCurrentPage()`, `copyToClipboard(QString)`. Maintains an
  article cache (`QMap<int, page>`).
- **`HistoryState`** (`src/state/HistoryState.h`): in-memory history vector
  (max 50 items) backed by `HistoryDatabase`. Emits `historyChanged`,
  `databaseError`, `databaseInitialized`.
- **`NavigationState`** (`src/state/NavigationState.h`): holds the `StackView`,
  a `std::map<QString, QQmlComponent*>` of named views, and an event filter
  that emits `backButtonPressed` on `Qt::BackButton`. `Q_INVOKABLE`
  `navigateToContent/History/Search/ImageGallery`, `navigateToView`,
  `addView`, `installEventFilter`.
- **`Constants`** (`src/constants.h`): `QML_SINGLETON` returning view-name
  strings (`"history"`, `"content"`, `"search"`, `"home"`, `"imageGallery"`).

### Wikipedia API clients (`src/wikipedia_*_client.{h,cpp}`)
All use `QNetworkAccessManager` and signal/slot async patterns.
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

- **Framework:** Qt Test (`QtTest/QtTest`, `QSignalSpy`, `QTEST_MAIN`).
- **Runner:** CTest (`enable_testing()` in `tests/CMakeLists.txt`).
- **Location:** `tests/`.
- **Test targets:** `GlobalStateTest`, `test_wikipedia_client`, `simple_test`,
  `verbose_test`, `basic_test`, `test_search_display`, `test_sidebar_layout`.
- Tests compile selected `.cpp` sources directly (e.g.
  `${CMAKE_SOURCE_DIR}/src/state/GlobalState.cpp`) rather than linking the
  full app.
- Run: `ctest --test-dir build` (or `./test.sh`).
- Note: `tests/CMakeLists.txt` references `src/wikipedia_client.cpp`, which no
  longer exists (the client was split into `wikipedia_search_client.cpp`,
  `wikipedia_page_client.cpp`, etc.). Those test targets may fail to build
  until updated; `GlobalStateTest`, `test_search_display`, and
  `test_sidebar_layout` reference current paths.

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
- `.continue/rules/` — Continue IDE rule files (most are `alwaysApply: true`):
  `c++.md`, `qt-database-error-handling.md`, `qt-event-filter-best-practices.md`,
  `qt-sqlite-best-practices.md`, `qt-stackview-navigation.md`,
  `qt-stackview-navigation-standards.md`,
  `commit-message-formatting-for-class-names.md`.
- `.vibe/agents/cpp.toml` + `.vibe/prompts/cpp.md` — Vibe "cpp" subagent with
  `web_search` tool; prompt mirrors the C++/Qt guidelines above.

## 10. Configuration

- No `.env` / `.env.example`. No runtime config file.
- API base URLs are hardcoded in the client constructors:
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
