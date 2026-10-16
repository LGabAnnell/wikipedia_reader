# Improvement opportunities

Reviewed: 2026-10-16. Source baseline: `45c773a`.

This report is based on static inspection of the current application, build configuration, and tests. No application code was changed, and no build, runtime reproduction, or test suite was run for this review. Findings below distinguish directly observed implementation problems from proposed hardening work. File links are relative to this document; symbol names identify the relevant code.

## Priorities

| Priority | Opportunity | Main benefit |
| --- | --- | --- |
| High | Isolate language changes and stale replies | Prevent articles from the wrong Wikipedia edition |
| High | Make history identities language-aware | Open the correct article from history |
| High | Enforce persistent history limits and write consistency | Bound storage and avoid misleading history state |
| High | Add request deadlines and validate API responses | Recover from stalled or invalid responses |
| Medium | Replace nested network event loops | Simplify cancellation and avoid reentrant request handling |
| Medium | Complete home-screen feedback and featured handling | Make failures visible and eliminate wasted requests |
| Medium | Bound article and math-image caches | Control memory usage during long sessions |
| Medium | Automate regression checks in CI | Catch failures before merging |
| Low | Refresh developer documentation and test tooling | Make setup reproducible and preserve build output |

## 1. Treat language changes as complete state transitions

**Observed:** [ArticleState.cpp](../src/state/ArticleState.cpp) updates its client's language and clears the cache, but retains the current article. [SearchState.cpp](../src/state/SearchState.cpp) changes the client language without clearing results. [HomeModel.cpp](../src/modules/HomeModule/HomeModel.cpp) updates three clients without clearing or refreshing cards. Client callbacks do not carry a language generation or reject obsolete replies.

**Impact:** Existing result and home cards retain page IDs from the old edition while subsequent loads use the new edition. An old title-resolution response can also initiate a page request against the newly selected host. Old article responses can repopulate a cache after it was cleared. Overlapping article loads in the same language can similarly publish results in completion order rather than selection order.

**Recommended change:** Introduce request context containing language and a generation/request ID. Invalidate it on language changes and superseding article selections; check it before emitting results, errors, or starting chained requests. Abort obsolete replies as an optimization. Clear language-bound views and refresh home data. Define whether previously opened article views preserve their original language or reset, then implement that consistently.

**Validation:** Use deferred fixtures to complete an English request after switching to French, and to finish article A after article B. Verify that stale results and errors are ignored, stale resolution does not start another request, and rapid language changes leave only the final edition visible.

**Existing work:** [The language isolation plan](language-change-state-isolation-plan.md) already describes much of this, but references the removed `GlobalState`. Update it for `SettingsState`, `ArticleState`, `SearchState`, and feature models. `ArticleStateTest::languageChangeInvalidatesCacheWithoutReloading` currently asserts that old content remains visible, so changing that behavior requires an explicit test update.

## 2. Include language in browsing-history identity

**Observed:** [HistoryDatabase.cpp](../src/state/db/HistoryDatabase.cpp) stores `page_id`, title, and timestamp, and deduplicates solely by page ID. [HistoryState.cpp](../src/state/HistoryState.cpp) uses the same identity. [History.qml](../src/modules/HistoryModule/History.qml) opens entries through `ArticleState.loadArticleByPageId()` using the active language.

**Impact:** Entries do not retain the edition they came from. Switching languages can open an unrelated article, and identical IDs from different editions overwrite one another.

**Recommended change:** Store `(language, page_id)` as the article identity, with a unique database index and an explicit schema migration. Either filter history by active language or display the edition and restore it when opening an entry. Decide how legacy rows with unknown language should be handled; do not silently assign an unverified edition.

**Validation:** Save identical IDs in two languages, restart, and verify independent entries and correct destinations. Cover migration, deduplication, and the chosen clear-history scope.

## 3. Fix history retention and database failure semantics

**Confirmed control-flow defect:** In [HistoryState.cpp](../src/state/HistoryState.cpp), `addToHistory()` removes the last in-memory item when the size exceeds `MAX_HISTORY_ITEMS`, then tests that same size again before calling `removeOldestItems()`. The second condition is false after trimming. Consequently, normal additions do not enforce the database limit.

**Related issue:** Both adding and clearing history mutate memory before checking database success. A failed clear can make the UI appear empty while the records remain on disk and return after restart. Database initialization also runs inside the `HistoryDatabase` constructor, before `HistoryState` connects its forwarding signals, so those initial notifications cannot be forwarded through those connections.

**Recommended change:** Make insertion and retention pruning one transaction. Publish the resulting in-memory state only after successful persistence, or explicitly expose an unsaved state. Connect initialization observers before initialization, or expose initialization status and error properties that callers can inspect afterward.

**Validation:** Add more than 50 distinct items through `HistoryState`, inspect the stored row count, and reload. Inject insertion, clearing, and initialization failures and verify visible state and error reporting. Existing database-helper tests do not replace a state-level retention regression test.

## 4. Bound requests and distinguish invalid responses from empty results

**Observed:** [The shared network manager](../src/wikipedia_network_access_manager.h) applies an HTTP/2 compatibility policy but no request timeout. The Wikipedia clients contain no explicit deadline/cancellation policy. Article completion also waits for all requested math images.

[WikipediaSearchClient.cpp](../src/wikipedia_search_client.cpp) emits `searchCompleted()` on transport success even when JSON is malformed or the expected response structure is missing. `parseSearchResults()` converts those cases into an empty vector. [WikipediaSearchClientTest.cpp](../tests/WikipediaSearchClientTest.cpp) explicitly expects zero results for malformed JSON and `{}`.

**Impact:** A stalled request can keep controls busy indefinitely. Invalid JSON or a JSON API error can appear as a successful “no results” search. Similar unchecked parsing patterns occur in the page and home clients and should be audited.

**Recommended change:** Add a shared transfer timeout and an overall deadline for multi-stage loads. Treat optional math-image failures as a fallback rather than an unbounded prerequisite. Validate JSON parse status, API error objects, and required fields before emitting success. Keep valid empty responses distinct from failures, and ensure every request ends with a defined success, failure, or cancellation outcome. Consider bounded retries only after cancellation and error semantics are reliable.

**Validation:** Cover noncompleting replies, malformed JSON, HTTP-success API errors, missing fields, valid empty results, and retry recovery. Update the tests that currently encode malformed responses as successful emptiness.

## 5. Replace synchronous image metadata lookup with an asynchronous chain

**Observed:** `WikipediaPageClient::fetchImageUrlsFromTitles()` in [wikipedia_page_client.cpp](../src/wikipedia_page_client.cpp) runs a local `QEventLoop::exec()` and fills output references after it exits. Its caller is already a network completion handler. Metadata-request failure is silently converted into an image-less result.

**Impact:** The nested loop processes other events while the original callback is still on the stack. That complicates navigation, object lifetime, cancellation, and language changes; it is not equivalent to simply blocking all UI event processing.

**Recommended change:** Return results through a callback or signal with owned request data and captured request context. Define whether metadata failure is a recoverable partial result or a user-visible error. Retain the fake network seam.

**Validation:** Defer metadata completion, change language or navigate away, and verify safe cancellation and no obsolete result. Test failure and empty-image cases without a nested loop.

## 6. Complete the home experience and featured-article pipeline

**Observed:** [HomeScreen.qml](../src/modules/HomeModule/HomeScreen.qml) renders news, on-this-day, and did-you-know sections, while `HomeModel::fetchHomeData()` also fetches a featured article and its images. The featured properties have no consumer in that screen. `HomeModel` exposes no loading/error status; only the featured client's error is connected to `handleError()`, which logs a warning. Errors from its home and page clients are not connected there.

**Confirmed title mutation:** `handleFeaturedArticleReceived()` calls `m_featuredArticleTitle.replace(' ', '_')` while building the URL. This mutates the stored display title. `handlePageWithImagesReceived()` subsequently compares a returned title against that mutated value, so ordinary titles containing spaces can fail the comparison.

**Recommended change:** Decide whether featured content belongs on the home screen. If it does, render it and fix URL construction using a separate title value and proper URL handling. Otherwise, remove the unused fetch chain. Expose loading, empty, error, and retry state per section so one failed feed does not obscure successful sections. Connect all relevant errors and refresh on language change.

**Validation:** Add a dedicated `tst_home.qml` using fixtures for successful feeds, partial failures, empty data, retry, and language changes. Test a featured title containing spaces and URL-sensitive characters. Existing QML tests cover several other features, but there is no dedicated home test file.

## 7. Put budgets on caches and math-image work

**Observed:** [ArticleState.h](../src/state/ArticleState.h) holds an unbounded `QMap<int, page>` cache. [WikipediaPageClient](../src/wikipedia_page_client.cpp) caches rasterized math images as base64 data URLs and starts a request for every distinct uncached formula in an article. Individual raster dimensions are capped at 4096, but there is no aggregate cache budget or application-level work queue.

**Opportunity:** Long sessions and formula-heavy articles can accumulate HTML and image data. The extent of the memory and responsiveness cost needs measurement; this review did not profile it.

**Recommended change:** Add bounded cache eviction, preferably by estimated bytes as well as entry count. Limit concurrent math-image processing and deduplicate in-flight work across article requests. Measure HTML processing and SVG conversion before deciding whether to move CPU work off the GUI thread.

**Validation:** Browse a large fixture set repeatedly and record memory and load latency. Assert eviction, refetch behavior, and the configured concurrency limit.

## 8. Add reproducible CI around the existing tests

**Observed:** No tracked `.github/workflows` configuration is present. [tests/CMakeLists.txt](../tests/CMakeLists.txt) already registers backend and QML tests, and the fixture transport provides a foundation for deterministic network behavior. This observation does not rule out externally configured CI.

**Recommended change:** Add a Linux job that provisions Qt, tinyxml2, and Lexbor, configures with `BUILD_TESTING=ON`, builds, and runs `ctest --test-dir build --output-on-failure` with an offscreen platform available. Account for GUI-backed C++ tests as well as QML tests. Keep live-network tests separate and opt-in. Upload failure logs; consider a relocated-install smoke test because the project now has deployment rules and a shared core library.

**Validation:** Demonstrate a clean checkout build and a failing-test run that fails the job and preserves useful diagnostics. Document the supported dependency baseline, then expand platform coverage as needed.

## 9. Refresh documentation and make test execution less disruptive

**Observed:** No tracked README is present. `AGENTS.md` and [improvement-observations.md](improvement-observations.md) still describe `GlobalState` and previously duplicated backend compilation. Those have been replaced by separate states and `wikipedia_qt_core`. The build now requires `lexbor 3.0`, absent from the supplied project overview. Testing guidance is split across `TESTING.md` and documents under `docs/`.

[test.sh](../test.sh) now deletes `build`, configures, and rebuilds with a hardcoded `-j14`; the agent guide describes it as only running CTest. The script does not enable fail-fast shell behavior, so a configuration/build failure does not immediately stop later steps.

**Recommended change:** Add a concise README with actual dependencies and build/run/test instructions. Consolidate authoritative testing guidance, update the agent guide, and label older proposals with implementation status. Make incremental test execution the default, with an explicit clean-build option, configurable parallelism, and immediate failure propagation. CMake presets could provide shared debug/test configurations.

**Validation:** Follow the documented steps from a fresh checkout. Confirm the normal test command preserves existing build output and returns failure when configuration or compilation fails.

## Suggested implementation sequence

1. Fix persistent history trimming and add its state-level regression test.
2. Implement language-aware identity and request generations together with the schema migration and deferred-response tests.
3. Add deadlines and response validation; replace the nested metadata event loop.
4. Complete home-screen state and coverage, including the featured-title fix.
5. Establish CI, refresh setup documentation, and profile caches before selecting budgets.

The existing shared backend, separated state classes, and fake network transport are useful foundations. Preserve those improvements; the older recommendations to introduce a shared core and split `GlobalState` should no longer be treated as outstanding work.
