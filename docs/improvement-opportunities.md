# Improvement opportunities

Updated: 2026-10-24, after implementation of GitHub issue #17. The formula
workload tests, full CTest suite, and relocated-install smoke passed. Of 32 CTest
entries, 31 passed and the opt-in `WikipediaFeedLiveTest` was skipped. The
GitHub Actions workflow has not been run remotely.
Coverage and test commands are documented in [the testing guide](full_testing.md).

## Remaining priorities

| Priority | Opportunity | Main benefit |
| --- | --- | --- |
| Low | Consolidate developer documentation and test tooling | Make setup reproducible and preserve build output |
| Low | Let QML create independent singleton services | Reduce application startup wiring |

## 1. History retention and database failure semantics — completed

History now retains at most 50 persisted entries globally across editions and legacy
unknown-edition rows. Initialization prunes oversized storage. Visit insertion,
retention, and snapshot loading share a transaction; state publishes only committed
results. Failed additions and clears preserve visible entries and support retry.
The History screen displays live and retained startup errors and an unavailable state.

Existing coverage: `HistoryStateTest`, `HistoryDatabaseTest`, and `QmlTests.tst_history`
cover retention, recreation, deterministic ordering, trigger-injected rollback,
retry, initialization failure, and UI feedback. See
[the testing guide](full_testing.md) for coverage limits.
SQLite file compaction and automatic initialization retries remain outside scope.

## 2. Bound request duration and validate responses — implemented

**Completed:** The active search, article HTML, gallery-page, and sections handlers
now reject invalid responses through `errorOccurred` rather than emitting a
successful empty result:

- Search (`fc8bace`) checks JSON parse status, API error objects, and the required
  `query.search` array. Valid empty arrays remain successful. The permissive static
  `parseSearchResults()` helper still returns an empty vector for invalid input;
  its parser-only tests do not describe the network handler's behavior.
- Article HTML (`42faa8b`) checks JSON parse status, API errors, and a string
  `parse.text`; an empty string is valid. Page-ID loading also rejects missing or
  invalid page identity before requesting HTML (`69ded9b`), and unresolved title
  lookups report an error (`0a0bb29`).
- Gallery-page and sections responses (`a8326a3`) check JSON parse status, API
  errors, and their required page/sections structures. A gallery must contain the
  requested valid page with a nonblank title. A valid page without images and an
  empty sections array remain successful.

The featured, news, on-this-day, and Did-you-know home pipelines also validate
their required structures. These checks do not establish strict validation of
every field in every client; image metadata handling is described below.

**Existing coverage:** [QmlTests.tst_search](../tests/qml/tst_search.qml) covers
malformed/structurally invalid search responses, API errors, valid empty results,
and retry recovery. [WikipediaPageClientTest](../tests/WikipediaPageClientTest.cpp)
covers invalid article HTML, valid empty text, missing page identity, unresolved
titles, and invalid/valid-empty gallery and section responses.
[ArticleStateTest](../tests/ArticleStateTest.cpp) verifies that invalid HTML is
neither cached nor recorded in history and permits retry. QML gallery/section
tests verify that HTTP-200 API errors stop loading and display feedback.

**Completed:** Production GETs use a 15-second inactivity timeout and a 45-second
monotonic operation deadline, including queue, retry, and follow-up stages. The
summary preview keeps its 10-second budget. HTTP cooldowns are scoped to the
scheme/host/port origin. Optional formula enrichment stops after three seconds
and publishes article fallback content. A deterministic local HTTP server covers
timeouts, queue deadlines, cooldowns, retries, concurrency, abort cleanup, and
slot recovery. Article metadata-to-HTML requests are asserted to share one
deadline. These changes do not establish deadline assertions for every client
follow-up chain.

## 3. Validate image metadata and preserve missing-file behavior — implemented

**Current behavior:** [fetchImageUrlsFromTitles()](../src/wikipedia_page_client.cpp)
already emits an error for transport failures, malformed JSON, and API error keys,
then returns without publishing success. Selected-image detail failures fall back
to the thumbnail/caption through `ImageSelectionState`. Valid metadata preserves
requested image order and skips missing files or entries without a usable URL;
`WikipediaPageClientTest` covers these cases. Initial gallery-page validation and
valid empty galleries are covered separately, as described above.

**Completed:** The metadata envelope and returned page/image-info objects are
validated, API errors have nonempty fallbacks, and entries with no usable URL or
legitimate missing files are skipped. Tests cover invalid envelopes, missing
files, no-usable-image results, metadata retry, and stale metadata replies.
Selected-image detail failures continue to fall back to the thumbnail/caption.

## 4. Budget math-image work and cache payloads — implemented

**Completed:** Formula enrichment is limited to 128 distinct uncached formulas
per article, two concurrent downloads, a three-second budget, 1 MiB per response,
and four million raster pixels. The formula LRU holds at most 256 entries / 16 MiB.
The article LRU holds at most 20 entries / 32 MiB payload. Four formula workload
tests verified entry and byte eviction, fanout, and timeout fallback. Across four
loads, the cache settled at 256 entries / 72,704 bytes with no pending replies;
an 853-byte injected budget retained three entries / 852 bytes. The tiny 2×2
formula fixture used 41,232 KiB maximum process RSS and has no pre-change baseline.
See [measured limits](full_testing.md#runtime-request-and-gallery-limits).

**Limit:** These fixture results verify bounded work and cached payload bytes;
they do not establish general process RSS or navigation-retained content bounds.

## 5. Add reproducible CI around the existing tests — implemented

**Completed:** A Linux workflow provisions the documented dependencies, configures
and builds tests, runs offscreen CTest, and performs an isolated relocated-install
startup smoke test. The relocated smoke passed without `LD_LIBRARY_PATH`. The
production queue tests use a deterministic loopback HTTP server. Local final
verification passed 31 of 32 CTest entries, with the opt-in live feed test skipped.
The workflow itself has not yet run remotely.

## 6. Make test execution less disruptive

**Completed:** Removed the legacy root test script, which always deleted `build/`, in favor of the incremental [runner](../scripts/run_tests.sh). Debug and release build helpers now live under `scripts/` with explicit names and separate build trees. Documentation points to [full_testing.md](full_testing.md) as the authoritative testing guide; completed proposals are indexed as historical in [the docs index](README.md).

**Remaining issue:** The incremental runner hardcodes `-j14`. CMake presets could provide shared debug/test configurations.

**Validation:** Follow the documented steps from a fresh checkout and confirm the runner preserves existing build output by default.

## 7. Let QML create independent singleton services

**Observed:** [main.cpp](../src/main.cpp) constructs and registers every QML singleton instance. `ClipboardHelper` and `NavigationState` have no constructor dependencies and are used from QML; neither needs an application-owned instance. They are candidates for creation by the QML engine. The remaining states have constructor dependencies, provide dependencies to application-owned states, or are accessed by C++ models through static `instance()` pointers, so changing their ownership also requires a coordinated initialization and access redesign.

**Recommended change:** Register `ClipboardHelper` and `NavigationState` as declarative `QML_SINGLETON` types in the `wikipedia_qt` module, with a `create(QQmlEngine *, QJSEngine *)` factory for each. Remove their `qmlRegisterSingletonInstance()` calls and stack instances from `main.cpp` and the corresponding explicit registrations in the QML test harness. Ensure the module's QML type registrar includes these types; adding `QML_SINGLETON` to a header alone is insufficient. Qt can use a default constructor for these classes, but explicit factories make engine ownership and future setup clear. Keep application-owned, dependency-injected states on explicit instance registration until their C++ consumers and initialization order are redesigned.

**Validation:** Run the offscreen QML tests and verify that navigation, the mouse Back button, and copying article text still work. Check that each engine gets its own singleton instance and that teardown does not leave an event filter pointing at a destroyed object.

## Suggested implementation sequence

1. Completed: persistent history retention, write consistency, and regression coverage.
2. Completed: shared request deadlines, strict metadata handling, bounded math enrichment and caches, gallery pagination, and Linux CI with relocated-install smoke coverage.
3. Consolidate setup/test documentation and runners.
4. Migrate the independent QML singleton services when changing startup wiring.
