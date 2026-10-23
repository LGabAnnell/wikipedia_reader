# Improvement opportunities

Reviewed: 2026-10-23. Source baseline: `a8326a3`.

Entries below distinguish completed work from remaining opportunities based on
targeted source, test, and commit-history inspection. This refresh did not rerun
the test suite. Coverage and test commands are documented in
[the testing guide](full_testing.md).

## Remaining priorities

| Priority | Opportunity | Main benefit |
| --- | --- | --- |
| High | Add transfer timeouts and overall request deadlines | Recover from stalled requests and bound queue/retry waits |
| Low | Tighten image-metadata validation and add failure/stale-stage coverage | Distinguish invalid metadata from valid missing images |
| Medium | Budget math-image memory and processing; assess article-cache byte limits | Control memory and processing costs beyond existing count/concurrency limits |
| Medium | Automate regression checks in CI | Catch failures before merging |
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

## 2. Bound request duration — response validation substantially completed

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
every field in every client; image metadata has specific remaining gaps below.

**Existing coverage:** [QmlTests.tst_search](../tests/qml/tst_search.qml) covers
malformed/structurally invalid search responses, API errors, valid empty results,
and retry recovery. [WikipediaPageClientTest](../tests/WikipediaPageClientTest.cpp)
covers invalid article HTML, valid empty text, missing page identity, unresolved
titles, and invalid/valid-empty gallery and section responses.
[ArticleStateTest](../tests/ArticleStateTest.cpp) verifies that invalid HTML is
neither cached nor recorded in history and permits retry. QML gallery/section
tests verify that HTTP-200 API errors stop loading and display feedback.

**Remaining issue:** [The shared network manager](../src/wikipedia_network_access_manager.h)
still sets no transfer timeout. Its [queued GET replies](../src/wikipedia_queued_reply.h)
limit active downloads to three and retry HTTP 429/503 at most twice, but have no
overall deadline covering queue waits, Retry-After pauses, and transfers. Clients
support cancellation and reject obsolete generations. Article completion still
waits for all requested math images; completed failures retain the original image
source, but a stalled reply can hold up the article. News lookback is bounded by
request count rather than elapsed time.

**Recommended change:** Add shared transfer timeouts and overall deadlines for
queued and multi-stage loads. Bound optional math-image waits and retain fallback
content when their budget expires. Preserve generation checks and define how
timeouts terminate each load's busy state.

**Validation to add:** Noncompleting replies, queue/retry deadline exhaustion,
multi-stage timeout cleanup, optional math-image fallback, and retry recovery
after timeout.

## 3. Tighten image-metadata validation and coverage

**Current behavior:** [fetchImageUrlsFromTitles()](../src/wikipedia_page_client.cpp)
already emits an error for transport failures, malformed JSON, and API error keys,
then returns without publishing success. Selected-image detail failures fall back
to the thumbnail/caption through `ImageSelectionState`. Valid metadata preserves
requested image order and skips missing files or entries without a usable URL;
`WikipediaPageClientTest` covers these cases. Initial gallery-page validation and
valid empty galleries are covered separately, as described above.

**Remaining issue:** Non-object JSON or missing/incorrectly typed `query.pages`
can still pass metadata handling as a successful result with no resolved images.
An API error without `info` can emit an empty error message. Gallery metadata
failure and metadata-stage stale replies lack dedicated regression coverage.

**Recommended change:** Validate the metadata envelope and provide nonempty error
fallbacks while preserving legitimate missing-file results and selected-image
fallback behavior. Retain the asynchronous implementation and fake network seam.

**Validation to add:** Defer the gallery metadata-stage reply specifically, then
switch language or supersede the gallery target and verify no obsolete result.
Cover metadata transport/API failures, invalid envelopes, valid responses with no
usable images, and retry recovery. Existing stale-gallery coverage defers the
initial page reply; selected-image detail staleness is already covered.

## 4. Budget math-image memory and processing — article LRU completed

**Completed:** [ArticleState](../src/state/ArticleState.h) caps its article cache
at 20 entries with least-recently-used eviction (`b43af97`). ID and resolved-title
cache hits refresh recency; language transitions clear both cache and recency.
[ArticleStateTest](../tests/ArticleStateTest.cpp) covers eviction/refetch, both hit
paths, and updates to existing entries without consuming additional capacity.
The shared download queue already limits active GET transfers to three (`e6e8aa5`).

**Remaining issue:** [WikipediaPageClient](../src/wikipedia_page_client.cpp)
caches rasterized math images as base64 data URLs without a byte or entry budget.
It creates queued requests for every distinct uncached formula in an article;
there is no bound on queued formula work or cross-request in-flight deduplication.
Raster dimensions are capped at 4096 per axis, but synchronous HTML processing and
SVG conversion still need profiling. The 20-entry article limit does not bound
total HTML bytes, including embedded math-image data.

**Recommended change:** Measure memory and load latency before selecting math-cache
budgets and deciding whether article-cache byte accounting is needed. Add bounded
math-cache eviction and queued-work budgets, and deduplicate in-flight formula
work across requests. Use profiling to decide whether CPU work should move off
the GUI thread.

**Validation to add:** Repeated formula-heavy fixture loads with memory/latency
measurements, math-cache eviction/refetch checks, queued-work and deduplication
assertions, and coverage of the existing shared transfer concurrency limit.

## 5. Add reproducible CI around the existing tests

**Observed:** No tracked `.github/workflows` configuration is present. [tests/CMakeLists.txt](../tests/CMakeLists.txt) already registers backend and QML tests, and the fixture transport provides a foundation for deterministic network behavior. This observation does not rule out externally configured CI.

**Recommended change:** Add a Linux job that provisions Qt, tinyxml2, and Lexbor, configures with `BUILD_TESTING=ON`, builds, and runs `ctest --test-dir build --output-on-failure` with an offscreen platform available. Account for GUI-backed C++ tests as well as QML tests. Keep live-network tests separate and opt-in. Upload failure logs; consider relocated-install and Flatpak smoke tests because deployment rules, a shared core library, and local Flatpak packaging are already present. Packaging support does not establish automated CI coverage.

**Validation:** Demonstrate a clean checkout build and a failing-test run that fails the job and preserves useful diagnostics. Document the supported dependency baseline, then expand platform coverage as needed.

## 6. Refresh documentation and make test execution less disruptive

**Remaining issues:** The legacy [test.sh](../test.sh) deletes `build`, rebuilds with `-j14`, and lacks fail-fast behavior. The incremental [runner](../scripts/run_tests.sh) also hardcodes `-j14`. Documentation now points to [full_testing.md](full_testing.md) as the authoritative testing guide; completed proposals are indexed as historical in [the docs index](README.md).

**Recommended change:** Replace or delegate the legacy `test.sh` to the incremental runner and make build parallelism configurable. CMake presets could provide shared debug/test configurations.

**Validation:** Follow the documented steps from a fresh checkout. Exercise the incremental runner's configuration/build failure paths and confirm failure propagation; verify a unified root test command preserves existing build output by default.

## 7. Let QML create independent singleton services

**Observed:** [main.cpp](../src/main.cpp) constructs and registers every QML singleton instance. `ClipboardHelper` and `NavigationState` have no constructor dependencies and are used from QML; neither needs an application-owned instance. They are candidates for creation by the QML engine. The remaining states have constructor dependencies, provide dependencies to application-owned states, or are accessed by C++ models through static `instance()` pointers, so changing their ownership also requires a coordinated initialization and access redesign.

**Recommended change:** Register `ClipboardHelper` and `NavigationState` as declarative `QML_SINGLETON` types in the `wikipedia_qt` module, with a `create(QQmlEngine *, QJSEngine *)` factory for each. Remove their `qmlRegisterSingletonInstance()` calls and stack instances from `main.cpp` and the corresponding explicit registrations in the QML test harness. Ensure the module's QML type registrar includes these types; adding `QML_SINGLETON` to a header alone is insufficient. Qt can use a default constructor for these classes, but explicit factories make engine ownership and future setup clear. Keep application-owned, dependency-injected states on explicit instance registration until their C++ consumers and initialization order are redesigned.

**Validation:** Run the offscreen QML tests and verify that navigation, the mouse Back button, and copying article text still work. Check that each engine gets its own singleton instance and that teardown does not leave an event filter pointing at a destroyed object.

## Suggested implementation sequence

1. Completed: persistent history retention, write consistency, and regression coverage.
2. Add request deadlines and tighten image-metadata validation, extending deferred-response coverage at the metadata stage. Search/article HTML/gallery-page/section response validation is already implemented.
3. Establish CI, consolidate setup/test documentation and runners, and profile math-image memory/processing before selecting further budgets. Article caching already has a 20-entry LRU limit and GET transfers already share a concurrency limit.
4. Migrate the independent QML singleton services when changing startup wiring.
