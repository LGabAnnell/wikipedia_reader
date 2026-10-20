# Full Code Review: `src/`

Fresh full audit of every file under `src/` (about 7,400 lines across ~50 C++/QML
files), reconciled with commit `8593ed0` on 2026-10-20. Earlier resolved findings
are retained below with their recorded validation; all still-open findings were
re-verified against the current sources. This is a static review only — no
builds or test runs were performed for this audit. See
[the testing guide](full_testing.md) for the coverage inventory.

## Critical

No critical findings. Notably clean: SQL is fully bound and parameterized,
the Wikimedia User-Agent policy is honored, no secrets are handled, and no
data-loss paths were found in normal flows.

## High

### `src/state/NavigationState.h:21` — `currentView()` is declared but never defined

The header declares `QString currentView() const;` and a
`currentViewChanged()` signal, but `NavigationState.cpp` defines neither the
method nor any emission of the signal. Any caller would fail to link. Define it
from the StackView's current item, or delete the declaration and signal.

### `src/state/NavigationState.h:49` — Missing `#include <map>` for `std::map`

The header declares `std::map<QString, QQmlComponent *> views;` without
including `<map>`. It compiles only via transitive includes — not portable.

## Medium

### `src/wikipedia_page_client.cpp:172` — Dead `imageTitles` local in `getPageById()`

The method fetches `prop=extracts|images` with `imlimit=50`, populates a local
`QStringList imageTitles` from the response, then calls
`fetchPageContentWithImages()` without passing it. The `extract` from this
response is also overwritten later by `fetchPageContentWithImages`. The gallery
loads images separately via `getPageWithImages()`, so the `images` prop and
`imageTitles` extraction are purely wasted API bandwidth on every article load —
remove `prop=extracts|images` and `imlimit=50` from the query and delete the
dead code.

### `src/wikipedia_page_client.cpp:356` — Gallery image order follows JSON key order, not article order

`fetchImageUrlsFromTitles()` appends results while iterating the response's
`pages` object, whose keys `QJsonObject` sorts alphabetically — not the order of
the requested `imageTitles`. Gallery images therefore appear in file-title
order rather than article order. Iterate `imageTitles` and look up each result
instead.

### `src/wikipedia_featured_client.cpp:27` — Double `reply->deleteLater()`

`getFeaturedArticleOfTheDay()` calls `reply->deleteLater()` in its lambda, and
`onFeaturedArticleReply()` calls it again at the end. Qt handles the double
`deleteLater()` gracefully, but one of the two calls should be removed.

### `src/wikipedia_models.h:128` — `section` struct: `level` and `index` lack default initialization

```cpp
int level;    // uninitialized
int index;    // uninitialized
```

Every other numeric member in the file has `= 0`. Default-constructed `section`
objects have indeterminate values. Add `= 0` (or `= -1` for `index`) to match
the pattern used elsewhere.

### `src/modules/HistoryModule/HistoryModel.cpp:14` — `HistoryModel` is dead code

`History.qml` uses `HistoryState` exclusively; nothing else references this
class. It duplicates history logic with different semantics (no database, no
edition-aware dedup) and is still compiled and exposed as a QML element in
`wikipedia_qt.History`. Remove the class from the module.

### `src/state/NavigationState.h:15` — `NavigationState` lacks `QML_UNCREATABLE("Singleton")`

AGENTS.md requires state singletons registered via `qmlRegisterSingletonInstance`
to be `QML_UNCREATABLE("Singleton")`. QML can currently instantiate stray
`NavigationState` instances that operate without a stack view.

### `src/modules/HomeModule/NewsSection.qml:62` — API text inserted unescaped into RichText links

`modelData.title` is concatenated directly into `<a href=...>` markup. The same
pattern appears in `src/modules/HomeModule/OnThisDaySection.qml:53` and
`src/modules/HomeModule/DidYouKnowSection.qml:40`, while
`src/modules/HomeModule/FeaturedSection.qml:59` correctly escapes `&`, `<`, and
`>`. Any API-sourced title or event containing those characters corrupts the
rendered markup. Extract the escaping into a shared helper and apply it in all
four sections.

### `src/state/ArticleState.h:67` — Article cache is unbounded

`m_articleCache` clears only on language change; a long browsing session grows
memory indefinitely (each cached `page` holds the full processed HTML). Cap the
cache and trim to a bounded number of entries.

### `src/modules/ContentDisplayModule/ContentDisplay.qml:468` — Relative links passed to `Qt.openUrlExternally()`

Non-`/wiki/` links are handed to `Qt.openUrlExternally()` as-is, but many
MediaWiki hrefs are relative (`/w/index.php?...`, `#anchor`), which the OS URL
handler cannot open. Resolve relative URLs against the edition base URL first,
and ignore pure fragment links.

### `src/modules/HeaderModule/Header.qml:52` — User-facing QML strings are not translated

Most user-facing strings are hardcoded without `qsTr()` ("Back", "Home",
"Search", "History", "Retry", the home section titles, "Image not available",
and others), contradicting the AGENTS.md rule that user-facing strings go
through `tr()`/`qsTr()`. Only a few files (`History.qml`, `LanguageSelector.qml`,
`ImageFullscreenWindow.qml`) are translated.

### `src/modules/ContentDisplayModule/CMakeLists.txt:1` — Wrong file comment

Line 1 says `# HistoryModule CMakeLists.txt` — this is the ContentDisplayModule.

### `src/modules/HeaderModule/CMakeLists.txt:1` — Wrong file comment

Line 1 says `# HistoryModule CMakeLists.txt` — this is the HeaderModule.

## Low

### `src/wikipedia_home_client.cpp:253` — Full API response logged at `qWarning`

`parseOnThisDayEvents()` dumps the entire response payload into the log when
the shape is unexpected. Log a shape summary instead.

### `src/wikipedia_page_client.cpp:27` — `stripHtml()` handles only 6 HTML entities

`&amp;`, `&lt;`, `&gt;`, `&quot;`, `&#39;`, `&nbsp;` are decoded. Numeric
entities (`&#x27;`, `&#169;`), `&apos;`, and others are left as-is. For Wikipedia
image descriptions this is usually sufficient, but a `QTextDocument`-based
approach would be more complete.

### `src/wikipedia_page_client.cpp:271` — `parsePageWithImages()` stores image titles in the `imageUrls` field

The comment says "caller fetches URLs" — the caller extracts titles from
`imageUrls`, clears it, then calls `fetchImageUrlsFromTitles()`. This
overloading of `imageUrls` is fragile; the `page` struct already has an
`imageTitles` field that would make the intent explicit.

### `src/state/SearchState.cpp:14` — Direct `m_isSearching = false` bypasses the setter

The `languageChanged` handler assigns `m_isSearching` directly, skipping
`setIsSearching()` and its change signal. It is also dead code — the
`languageTransitionStarted` handler already cleared the flag via the setter
before `languageChanged` fires. Remove it.

### `src/modules/HeaderModule/HeaderModel.cpp:65` — Dead render path and no-op property

`renderer.setProperty("fill", "white")` only sets a dynamic property on
`QSvgRenderer`; it has no rendering effect. Neither `renderSvgToImage()` nor
`calculateSvgScaleFactor()` is called from QML, `SvgImageProvider`'s `m_model`
member is stored but never used, and `SvgImageProvider::setFillColor()` has no
callers. Trim the cluster; note that `requestImage()` runs on a QML worker
thread if `m_fillColor` ever becomes mutable state.

### `src/modules/HeaderModule/HeaderModel.cpp:51` — `renderSvgToImage()` reads the SVG file twice

`calculateSvgScaleFactor()` opens and reads the file, then `renderSvgToImage()`
opens and reads the same file again. Pass the loaded `QByteArray` or
`QSvgRenderer` between the two calls.

### `src/modules/ContentDisplayModule/ContentDisplayModel.h:94` — Unused `m_items` member

`m_items` is never used; the `searchRequested` signal declared at line 68 is
never emitted. Remove both.

### `src/state/db/HistoryDatabase.h:31` — `removeOldestItems()` has no callers

Pruning goes through `pruneHistory()` inside transactions. Remove the dead
public API.

### `src/modules/HomeModule/FeaturedSection.qml:38` — Magic status numbers in QML

`visible: root.status === 2` and `HomeFeedback.qml:15`'s `status === 1 || 3 || 4`
use magic numbers instead of the exposed `HomeModel.Status` enum values, which
the code already uses elsewhere (`homeModel.retrySection(HomeModel.Featured)`).

### `src/constants.h:42` — `Constants::create()` singleton ownership is undocumented

The static `m_instance` is never deleted and ownership is not handed to the QML
engine. It works as a deliberate one-instance singleton, but the ownership
should be documented or transferred explicitly.

### `src/Main.qml:124` — `homeView` registration is unreachable

`homeComponent` is registered with `NavigationState.addView()`, but every caller
intercepts `homeView` (`Header.qml`, `Main.qml`) and pops to root instead. The
registration is dead; a stray `navigateToView(homeView)` would push a second
HomeScreen on top of the stack.

### `src/CMakeLists.txt:20` — Resources registered through two mechanisms

`resources.qrc` plus `../icons/*.svg` are listed in `qt_add_qml_module` RESOURCES
while the same icons are also added via `qt_add_resources(appwikipedia_qt
"icons")`, and `styles/table_style.css` appears in both `resources.qrc` and the
`styles` resource. It works, but the duplication invites alias drift; pick one
mechanism per file.

### `src/SearchScreen.qml:6` — Inconsistent versioned imports

Uses `import wikipedia_qt 1.0` while most other files use unversioned
`import wikipedia_qt`. AGENTS.md notes versioned imports appear in "older
files" — worth normalizing.

## Resolved findings

These findings were fixed and are retained with their recorded validation,
including fixes made after the audit.

### `src/wikipedia_page_client.cpp` — `getPageById()` silently emitted an empty page when the ID was missing

The metadata response must now contain the requested page with a nonempty title
and no `missing` or `invalid` marker before HTML loading starts. Otherwise the
client emits a translated `errorOccurred` message and skips the HTML request.
Fixture regression tests cover empty pages, a different page ID, missing/invalid
markers, a missing title/query, malformed JSON, and successful metadata-to-HTML
loading. Failure cases verify one error, no page signal, and no follow-up request.
The empty-pages regression reproduced the unwanted HTML request before the fix.
Validation: `./scripts/run_tests.sh -R '^(WikipediaPageClientTest|WikipediaRequestIsolationTest|ArticleStateTest|QmlTests\.tst_article)$'`
passed (all four CTest entries).

### `src/wikipedia_network_access_manager.h` — `NetworkAccessManagerFactory::create()` could recurse into itself

The runtime factory now returns `new NetworkAccessManager(parent)` directly,
so installing it through `installNetworkAccessManagerFactory()` no longer
recurses through the factory lookup. Installed-factory dispatch remains in
`createNetworkAccessManager()`. Regression coverage exercises API-client and
QML-image manager creation both with and without the runtime factory installed
globally, verifies API-client QObject ownership, and checks local-file request
completion and the runtime HTTP/2 policy. Scoped cleanup restores the previous
factory property even when an assertion fails.
Validation: `./scripts/run_tests.sh -R '^WikipediaNetworkAccessManagerTest$'`
passed (all four data rows).

### `src/modules/ContentDisplayModule/ContentDisplay.qml` — In-article `/wiki/` links were not percent-decoded

The link handler now decodes the title with `decodeURIComponent()` before
replacing underscores with spaces. Malformed encodings log a warning and return
before freezing the view, loading an article, or navigating. QML regression
tests cover plain titles, encoded Unicode, percent signs, encoded underscores,
and incomplete, invalid, or malformed UTF-8 escapes. Successful links preserve
push/back navigation; rejected links preserve the current article, history,
request count, and live article bindings.
Validation: `./scripts/run_tests.sh -R '^QmlTests\.tst_article$'` passed.

### `src/modules/ImageDisplayModule/ImageGallery.qml` — Back button referenced external `stackView`

The Back handler now uses `root.StackView.view` and pops only when the owning
stack exists and has more than one item. The button exposes the stable
`imageGalleryBackButton` selector. Regression tests cover returning to the
previous item in a separately named stack, retaining a root-level gallery, and
safely clicking Back on a standalone gallery, with no QML warnings.
Previously recorded validation: `./scripts/run_tests.sh -R QmlTests.tst_images` passed.

### `src/state/NavigationState.cpp` — `navigateToView()` threw for unregistered view names

`navigateToView()` now uses `views.find(view)`, logs a warning containing an
unregistered name, and returns without emitting `pushView`. QML regression tests
cover empty and unknown names, unchanged stack depth/current item, and successful
registered-view navigation afterward.
Previously recorded validation: `./scripts/run_tests.sh -R '^QmlTests\.tst_navigation$'` passed.

### `src/state/NavigationState.cpp` — Same `views.at(view)` pattern in `setCurrentView()`

Removed the unused `setCurrentView()` slot, its `replaceView` signal, and the
corresponding connection in `Main.qml`, eliminating the throwing lookup.
Existing QML navigation tests cover invalid names, push/back navigation, and
returning Home. Validation:
`./scripts/run_tests.sh -R '^QmlTests\.tst_navigation$'` passed.

### `src/wikipedia_home_client.cpp` — `getOnThisDayEvents(int month, int day)` ignored its parameters

The request URL now uses the supplied `month` and `day` instead of
`QDate::currentDate()`, retaining two-digit formatting for both path segments.
`testOnThisDayUsesRequestedDate` in `tests/WikipediaHomeClientTest.cpp` covers
January 2 in English and December 31 in French using fixture responses. Each
case verifies one request, successful completion, and no error signal.
Validation: `ctest --test-dir build -R '^WikipediaHomeClientTest$' --output-on-failure` passed.

## Recent gallery coverage

The gallery navigation and image-details change (`e134cab`) has regression
coverage in `ImageSelectionStateTest` and `QmlTests.tst_images`: navigation stops
at either end, standalone selection clears gallery context, duplicate selection
does not repeat detail requests, failed/missing details fall back to the
thumbnail and caption, and stale details cannot overwrite a newer selection.
QML tests exercise arrow keys in embedded and fullscreen viewers, caption
updates, and shortcut isolation for covered viewers. Viewer preference
coverage checks gallery recreation within one process.

## What checked out clean

Request lifecycle is solid: `WikipediaRequestTracker` generation-guarding
correctly drops stale replies across language switches, `QueuedReply` handles
abort/destruction/rate-limit retries without double-release of the download
budget, and the SQLite layer follows the repo's stated best practices (named
connection, prepared statements, transactions with rollback, guarded
destructor). StackView navigation uses push/pop exclusively, history identity
`(language, pageId)` is enforced end to end, and the "No results found." label
behaves exactly as AGENTS.md specifies. The Lexbor HTML cleanup has disciplined
memory handling for a C API. No `console.log` calls exist in QML files.

## Summary

| Severity | Unresolved count |
|----------|-------------------|
| Critical | 0 |
| High     | 2 |
| Medium   | 13 |
| Low      | 14 |

The highest-value remaining fixes are the undefined `NavigationState::currentView()`
method (a linker failure for callers), and the dead `images` prop in
`getPageById()` (wasted API bandwidth on every article load).
The original audit was static; the resolved entries above record validation
performed for individual fixes.
