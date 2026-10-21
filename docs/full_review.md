# Full Code Review: `src/`

The original full audit covered every file under `src/` (about 7,400 lines
across ~50 C++/QML files) and was reconciled with commit `8593ed0` on
2026-10-20. On 2026-10-21, the listed findings were reassessed against commit
`e94c87e`; this was a targeted recheck, not a new full-source audit. Severity,
ownership, singleton construction, caller, and resource-packaging claims have
been corrected below. Earlier resolved findings retain their recorded validation
and remain consistent with the current sources.

The recheck included a successful incremental build, all 27 CTest entries, and
temporary probes for singleton construction and ownership. Live-network testing
was not enabled. See [the testing guide](full_testing.md) for coverage limits.

## Critical

No critical findings. Notably clean: SQL is fully bound and parameterized,
the Wikimedia User-Agent policy is honored, no secrets are handled, and no
data-loss paths were found in normal flows.

## High

No current high-severity findings. The two original High entries are retained
below as Low issues because neither prevents the current application from building.

## Medium

### `src/modules/HomeModule/NewsSection.qml:62` — API text inserted unescaped into RichText links

`modelData.title` is concatenated directly into `<a href=...>` markup. The same
pattern appears in `src/modules/HomeModule/OnThisDaySection.qml:53` and
`src/modules/HomeModule/DidYouKnowSection.qml:40`, while
`src/modules/HomeModule/FeaturedSection.qml:59` correctly escapes `&`, `<`, and
`>`. Any API-sourced title or event containing those characters corrupts the
rendered markup. Extract the escaping into a shared helper and apply it in all
four sections.

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

## Low

### `src/state/NavigationState.h:21` — `currentView()` is declared but never defined

The header declares `QString currentView() const;`, but there is no method
implementation or caller. `currentViewChanged()` is declared but never emitted;
Qt's meta-object compiler supplies the signal implementation. A future C++ call
to `currentView()` would fail to link, but the current application builds and
links successfully. Remove the unused method and signal, or implement the API
when needed. Downgraded from High to Low as a latent API issue.

### `src/state/NavigationState.h:49` — Missing `#include <map>` for `std::map`

The header declares `std::map<QString, QQmlComponent *> views;` without
including `<map>`. It builds through transitive includes in the current
environment, but should include its dependency directly. Downgraded from High
to Low as a portability and header-maintenance issue.

### `src/wikipedia_featured_client.cpp:27` — Double `reply->deleteLater()`

`getFeaturedArticleOfTheDay()` calls `reply->deleteLater()` in its lambda, and
`onFeaturedArticleReply()` calls it again at the end. Qt handles the double
`deleteLater()` gracefully, but one of the two calls should be removed.
Downgraded from Medium to Low; this is harmless redundant cleanup.

### `src/state/NavigationState.h:15` — Missing singleton annotation is a convention issue

AGENTS.md requires `QML_UNCREATABLE("Singleton")` on state singletons registered
via `qmlRegisterSingletonInstance`. The annotation is missing, but the runtime
registration in `main.cpp` and the QML test harness already prevents QML
construction. A temporary test using the existing harness confirmed that
`Qt.createQmlObject()` with `NavigationState {}` fails with "Element is not
creatable." Add the annotation for consistency; the earlier claim that QML can
instantiate stray instances is incorrect. Downgraded from Medium to Low.

### `src/modules/ContentDisplayModule/CMakeLists.txt:1` — Wrong file comment

Line 1 says `# HistoryModule CMakeLists.txt` — this is the ContentDisplayModule.
Downgraded from Medium to Low; this is cosmetic.

### `src/modules/HeaderModule/CMakeLists.txt:1` — Wrong file comment

Line 1 says `# HistoryModule CMakeLists.txt` — this is the HeaderModule.
Downgraded from Medium to Low; this is cosmetic.

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

### `src/modules/HomeModule/FeaturedSection.qml:38` — Magic status numbers in QML

`visible: root.status === 2` and `HomeFeedback.qml:15`'s comparisons against
`1`, `3`, and `4` use magic numbers instead of the exposed `HomeModel.Status` enum values, which
the code already uses elsewhere (`homeModel.retrySection(HomeModel.Featured)`).

### `src/Main.qml:124` — `homeView` registration is unreachable

`homeComponent` is registered with `NavigationState.addView()`, but every caller
intercepts `homeView` (`Header.qml`, `Main.qml`) and pops to root instead. The
registration is dead; a stray `navigateToView(homeView)` would push a second
HomeScreen on top of the stack.

### `src/CMakeLists.txt:28` — Duplicate icons and an embedded raw resource manifest

The generated `wikipedia_qt_raw_res_0.qrc` embeds four icons also included in
the application's `icons` resource, under different resource paths. Consolidate
the duplicated icon payloads and aliases.

The generated resource also embeds `resources.qrc` itself as raw data at
`:/qt/qml/wikipedia_qt/resources.qrc`; it does not expand that manifest into
another stylesheet resource. The stylesheet payload is supplied by the
application's `styles` resource, so the earlier claim that it is duplicated
through `resources.qrc` is incorrect. Remove the unnecessary raw manifest
entry if no consumer needs it. This remains a Low packaging cleanup.

### `src/SearchScreen.qml:6` — Inconsistent versioned imports

Uses `import wikipedia_qt 1.0` while most other files use unversioned
`import wikipedia_qt`. AGENTS.md notes versioned imports appear in "older
files" — worth normalizing.

## Retracted finding

### `src/state/db/HistoryDatabase.h:31` — `removeOldestItems()` has test callers

The earlier "has no callers" claim is incorrect. `HistoryDatabaseTest.cpp`
calls `removeOldestItems()` in explicit pruning and initialization-failure
coverage. Production pruning uses `pruneHistory()` within transactions, so the
public wrapper currently has only test consumers. Removing it would require a
deliberate API and test change; it is not counted as an unresolved defect.

## Resolved findings

These findings were fixed and are retained with their recorded validation,
including fixes made after the audit.

### `src/wikipedia_models.h` — `section` numeric fields lacked default initialization

`section::level` and `section::index` now default to zero, so default-constructed
sections have deterministic numeric values, consistent with the parser's
fallback values for absent fields.
Validation: incremental build and existing `WikipediaPageClientTest`,
`ContentDisplayModelTest`, `QmlTests.tst_sections`, and `QmlTests.tst_smoke` passed.
These checks cover parsing, section display, and QML registration; no dedicated
default-construction regression test was added.

### `src/modules/HistoryModule/HistoryModel.cpp` — Unused duplicate history model

Removed `HistoryModel.h` and `HistoryModel.cpp` and their `SOURCES` entries from
the History QML module. Source-reference inspection confirmed that no consumer
used the class; `History.qml` uses the persistent `HistoryState` singleton.
The module retains its `History.qml` component.
Validation: `./scripts/run_tests.sh -R '^(WikipediaPageClientTest|ContentDisplayModelTest|QmlTests\.tst_(history|navigation|sections|smoke))$'`
passed (incremental build and all six CTest entries), including history-screen
operations and navigation with the regenerated QML module.

### `src/wikipedia_page_client.cpp` — Gallery image order followed JSON key order instead of requested title order

`fetchImageUrlsFromTitles()` now indexes response pages by file title and
appends metadata in the requested `imageTitles` sequence. URLs, titles, and
descriptions remain aligned while missing pages, empty image info, and files
without a usable URL are skipped. Thumbnail preference and original-URL
fallback are preserved. A fixture regression requests Zebra before Alpha with
response page IDs that sort Alpha first; it failed on the first title before
the fix. It also verifies caption decoding, missing-caption alignment, skipped
files, and the original-URL fallback. Two existing image fixtures now include
the API's file-title field so metadata can be matched by title.
Validation: `./scripts/run_tests.sh -R '^(WikipediaPageClientTest|WikipediaRequestIsolationTest|ImageSelectionStateTest|QmlTests\.tst_(images|feature_isolation))$'`
passed (all five CTest entries).

### `src/wikipedia_page_client.cpp` — `getPageById()` fetched unused extracts and image titles

The initial query now requests only page identity, omitting `prop`,
`explaintext`, and `imlimit`. Removed the overwritten extract assignment and
unused image-title parsing; article HTML still comes from the subsequent parse
request, and galleries retain their separate image queries. Fixture regression
tests require the smaller query and verify successful metadata-to-HTML loading
from identity-only responses in English and French. The success regression
rejected the oversized request before the fix. Existing missing-page,
request-isolation, article-state, and QML fixtures now use the smaller query.
Validation: `./scripts/run_tests.sh -R '^(WikipediaPageClientTest|WikipediaRequestIsolationTest|ArticleStateTest|QmlTests\.tst_(article|history|navigation|images|feature_isolation))$'`
passed (all eight CTest entries).

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

The Back handler now uses `root.StackView.view`, with a parent attached-view
fallback, and pops only when the owning stack exists and has more than one
item. The button exposes the stable
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

## Recheck validation

On 2026-10-21 at commit `e94c87e`:

- `cmake --build build -j14` succeeded.
- `QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure`
  passed all 27 entries. The opt-in live search was not enabled; these results
  do not establish live Wikipedia API compatibility.
- A temporary QML test using the existing harness verified that
  `NavigationState {}` cannot be instantiated.
- A temporary C++ probe verified deletion of the factory-provided `Constants`
  singleton on engine destruction and the stale pointer returned afterward.
- Generated resource manifests confirmed duplicated icon payloads and raw
  embedding of `resources.qrc`, rather than a second stylesheet payload.

The temporary probes were outside the repository and were not added as
regression tests. Passing existing tests does not resolve the open findings;
several concern behavior those tests do not exercise.

## Summary

| Severity | Unresolved count |
|----------|-------------------|
| Critical | 0 |
| High     | 0 |
| Medium   | 3 |
| Low      | 17 |
| **Total** | **20** |

There are 20 unresolved entries and one retracted entry. The previous summary
claimed 25 unresolved findings, but its sections contained only 23.

Prioritize escaping API text in RichText links and resolving relative article
links.
