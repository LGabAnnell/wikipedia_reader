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

No current high-severity findings.

## Medium

No current medium-severity findings.

## Low

### `src/wikipedia_featured_client.cpp:27` — Double `reply->deleteLater()`

`getFeaturedArticleOfTheDay()` calls `reply->deleteLater()` in its lambda, and
`onFeaturedArticleReply()` calls it again at the end. Qt handles the double
`deleteLater()` gracefully, but one of the two calls should be removed.
Downgraded from Medium to Low; this is harmless redundant cleanup.

### `src/modules/ContentDisplayModule/CMakeLists.txt:1` — Wrong file comment

Line 1 says `# HistoryModule CMakeLists.txt` — this is the ContentDisplayModule.
Downgraded from Medium to Low; this is cosmetic.

### `src/modules/HeaderModule/CMakeLists.txt:1` — Wrong file comment

Line 1 says `# HistoryModule CMakeLists.txt` — this is the HeaderModule.
Downgraded from Medium to Low; this is cosmetic.

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
| Medium   | 0 |
| Low      | 5 |
| **Total** | **5** |

There are 5 unresolved entries and one retracted entry. The previous summary
claimed 25 unresolved findings, but its sections contained only 23.
