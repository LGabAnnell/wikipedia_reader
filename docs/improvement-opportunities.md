# Improvement opportunities

Reviewed: 2026-10-18. Source baseline: `cabd681`.

Completed work has been removed from this document. The remaining items are based on static inspection of the application, build configuration, and tests; no application code was changed and no test suite was run for this review.

## Remaining priorities

| Priority | Opportunity | Main benefit |
| --- | --- | --- |
| High | Enforce persistent history limits and write consistency | Bound storage and avoid misleading history state |
| High | Add request deadlines and validate API responses | Recover from stalled or invalid responses |
| Low | Define image-metadata failure semantics and add regression coverage | Make partial gallery results predictable |
| Medium | Complete home-screen feedback and featured handling | Make failures visible and eliminate unused work |
| Medium | Bound article and math-image caches | Control memory usage during long sessions |
| Medium | Automate regression checks in CI | Catch failures before merging |
| Low | Consolidate developer documentation and test tooling | Make setup reproducible and preserve build output |

## 1. Fix history retention and database failure semantics

**Confirmed control-flow defect:** In [HistoryState.cpp](../src/state/HistoryState.cpp), `addToHistory()` removes the last in-memory item when the size exceeds `MAX_HISTORY_ITEMS`, then tests that same size again before calling `removeOldestItems()`. The second condition is false after trimming. Consequently, normal additions do not enforce the database limit.

**Related issue:** Both adding and clearing history mutate memory before checking database success. A failed clear can make the UI appear empty while the records remain on disk and return after restart. Database initialization also runs inside the `HistoryDatabase` constructor, before `HistoryState` connects its forwarding signals, so those initial notifications cannot be forwarded through those connections.

**Recommended change:** Make insertion and retention pruning one transaction. Publish the resulting in-memory state only after successful persistence, or explicitly expose an unsaved state. Connect initialization observers before initialization, or expose initialization status and error properties that callers can inspect afterward.

**Validation:** Add more than 50 distinct items through `HistoryState`, inspect the stored row count, and reload. Inject insertion, clearing, and initialization failures and verify visible state and error reporting. Existing database-helper tests do not replace a state-level retention regression test.

## 2. Bound requests and distinguish invalid responses from empty results

**Observed:** [The shared network manager](../src/wikipedia_network_access_manager.h) applies the app User-Agent and an HTTP/2 compatibility policy but no request timeout. Clients expose explicit cancellation and reject obsolete generations, but there is still no deadline for an active request. Article completion waits for all requested math images; completed math-image failures retain the original source as a fallback, but a stalled reply can still hold up the article. The news lookback is bounded by request count, not elapsed time.

[WikipediaSearchClient.cpp](../src/wikipedia_search_client.cpp) emits `searchCompleted()` on transport success even when JSON is malformed or the expected response structure is missing. `parseSearchResults()` converts those cases into an empty vector. [WikipediaSearchClientTest.cpp](../tests/WikipediaSearchClientTest.cpp) explicitly expects zero results for malformed JSON and `{}`.

**Impact:** A stalled request can keep controls busy indefinitely. Invalid JSON or a JSON API error can appear as a successful “no results” search. Similar unchecked parsing patterns occur in the page and home clients and should be audited.

**Recommended change:** Add a shared transfer timeout and an overall deadline for multi-stage loads. Bound the wait for optional math images and retain fallback content when that budget expires. Validate JSON parse status, API error objects, and required fields before emitting success. Keep valid empty responses distinct from failures, and ensure every request ends with a defined success, failure, or cancellation outcome. Preserve the existing generation checks when adding deadlines or retries.

**Validation:** Cover noncompleting replies, malformed JSON, HTTP-success API errors, missing fields, valid empty results, and retry recovery. Update the tests that currently encode malformed responses as successful emptiness.

## 3. Define image-metadata failure behavior

**Remaining issue:** Metadata-request failure completes successfully with no resolved images and no error or partial-result indication. Unchecked JSON parsing can also silently produce an image-less result.

**Recommended change:** Define whether metadata failure is a recoverable partial result or a user-visible error, and validate the response accordingly. Retain the asynchronous implementation and fake network seam.

**Validation:** Defer the metadata-stage reply specifically, then switch language or supersede the gallery target and verify no obsolete result. Add explicit failure and empty-image cases; current stale-gallery coverage defers the initial page reply rather than metadata completion.

## 4. Complete the home experience and featured-article pipeline

**Observed:** [HomeScreen.qml](../src/modules/HomeModule/HomeScreen.qml) renders news, on-this-day, and did-you-know sections, while `HomeModel::fetchHomeData()` also fetches a featured article and its images. The featured properties have no consumer in that screen. `HomeModel` exposes no loading or error status; only the featured client's error is connected to `handleError()`, which logs a warning. Errors from its home and page clients are not connected there.

**Recommended change:** Decide whether featured content belongs on the home screen: render it or remove the unused fetch chain. Expose loading, empty, error, and retry state per section so one failed feed does not obscure successful sections. Connect all relevant errors.

**Remaining validation:** Add dedicated home integration coverage for pending, empty, error, and retry feedback and partial failures across sections, plus featured title/URL regression cases with spaces and URL-sensitive characters.

## 5. Put budgets on caches and math-image work

**Observed:** [ArticleState.h](../src/state/ArticleState.h) holds an unbounded `QMap<int, page>` cache. [WikipediaPageClient](../src/wikipedia_page_client.cpp) caches rasterized math images as base64 data URLs and starts a request for every distinct uncached formula in an article. Individual raster dimensions are capped at 4096, but there is no aggregate cache budget or application-level work queue.

**Opportunity:** Long sessions and formula-heavy articles can accumulate HTML and image data. The extent of the memory and responsiveness cost needs measurement; this review did not profile it.

**Recommended change:** Add bounded cache eviction, preferably by estimated bytes as well as entry count. Limit concurrent math-image processing and deduplicate in-flight work across article requests. Measure HTML processing and SVG conversion before deciding whether to move CPU work off the GUI thread.

**Validation:** Browse a large fixture set repeatedly and record memory and load latency. Assert eviction, refetch behavior, and the configured concurrency limit.

## 6. Add reproducible CI around the existing tests

**Observed:** No tracked `.github/workflows` configuration is present. [tests/CMakeLists.txt](../tests/CMakeLists.txt) already registers backend and QML tests, and the fixture transport provides a foundation for deterministic network behavior. This observation does not rule out externally configured CI.

**Recommended change:** Add a Linux job that provisions Qt, tinyxml2, and Lexbor, configures with `BUILD_TESTING=ON`, builds, and runs `ctest --test-dir build --output-on-failure` with an offscreen platform available. Account for GUI-backed C++ tests as well as QML tests. Keep live-network tests separate and opt-in. Upload failure logs; consider relocated-install and Flatpak smoke tests because deployment rules, a shared core library, and local Flatpak packaging are already present. Packaging support does not establish automated CI coverage.

**Validation:** Demonstrate a clean checkout build and a failing-test run that fails the job and preserves useful diagnostics. Document the supported dependency baseline, then expand platform coverage as needed.

## 7. Refresh documentation and make test execution less disruptive

**Remaining issues:** [AGENTS.md](../AGENTS.md) and [TESTING.md](../TESTING.md) still describe `GlobalState` and/or previously duplicated backend compilation. The README links to that outdated testing guide, and [full_testing.md](full_testing.md) has not yet incorporated the new news fallback/rendering coverage. Testing guidance remains split across those files and `docs/`. The build requires Lexbor 3.0, absent from the agent guide's dependency overview; the guide also incorrectly says no README exists. The legacy [test.sh](../test.sh) still deletes `build`, rebuilds with `-j14`, and lacks fail-fast behavior; the agent guide incorrectly describes it as only running CTest. The incremental runner also hardcodes `-j14`.

**Recommended change:** Consolidate authoritative testing guidance around the coverage guide, refresh its news coverage, update the agent guide, and label remaining older proposals with implementation status. Replace or delegate the legacy `test.sh` to the incremental runner and make build parallelism configurable. CMake presets could provide shared debug/test configurations.

**Validation:** Follow the documented steps from a fresh checkout. Exercise the incremental runner's configuration/build failure paths and confirm failure propagation; verify a unified root test command preserves existing build output by default.

## Suggested implementation sequence

1. Fix persistent history trimming and add its state-level regression test.
2. Add deadlines and response validation, define metadata partial-result semantics, and extend deferred-response coverage across the asynchronous chains.
3. Complete home-screen feedback and decide whether to render featured content; extend coverage to section failures/retry and featured title/URL behavior.
4. Establish CI, consolidate setup/test documentation and runners, and profile caches before selecting budgets.
