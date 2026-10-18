# Improvement opportunities

Reviewed: 2026-10-18. Source baseline: `2841adb`.

Entries below describe remaining work based on static inspection. Completed
home behavior is documented in [the testing guide](full_testing.md).

## Remaining priorities

| Priority | Opportunity | Main benefit |
| --- | --- | --- |
| High | Add request deadlines and validate API responses | Recover from stalled or invalid responses |
| Low | Define image-metadata failure semantics and add regression coverage | Make partial gallery results predictable |
| Medium | Bound article and math-image caches | Control memory usage during long sessions |
| Medium | Automate regression checks in CI | Catch failures before merging |
| Low | Consolidate developer documentation and test tooling | Make setup reproducible and preserve build output |
| Low | Let QML create independent singleton services | Reduce application startup wiring |

## 1. History retention and database failure semantics — completed

History now retains at most 50 persisted entries globally across editions and legacy
unknown-edition rows. Initialization prunes oversized storage. Visit insertion,
retention, and snapshot loading share a transaction; state publishes only committed
results. Failed additions and clears preserve visible entries and support retry.
The History screen displays live and retained startup errors and an unavailable state.

Validation: `HistoryStateTest`, `HistoryDatabaseTest`, and `QmlTests.tst_history`
cover retention, recreation, deterministic ordering, trigger-injected rollback,
retry, initialization failure, and UI feedback. The full offscreen CTest suite
passes all 27 entries. See [the testing guide](full_testing.md) for coverage limits.
SQLite file compaction and automatic initialization retries remain outside scope.

## 2. Bound requests and distinguish invalid responses from empty results

**Observed:** [The shared network manager](../src/wikipedia_network_access_manager.h) applies the app User-Agent and an HTTP/2 compatibility policy but no request timeout. Clients expose explicit cancellation and reject obsolete generations, but there is still no deadline for an active request. Article completion waits for all requested math images; completed math-image failures retain the original source as a fallback, but a stalled reply can still hold up the article. The news lookback is bounded by request count, not elapsed time.

[WikipediaSearchClient.cpp](../src/wikipedia_search_client.cpp) emits `searchCompleted()` on transport success even when JSON is malformed or the expected response structure is missing. `parseSearchResults()` converts those cases into an empty vector. [WikipediaSearchClientTest.cpp](../tests/WikipediaSearchClientTest.cpp) explicitly expects zero results for malformed JSON and `{}`.

**Impact:** A stalled request can keep controls busy indefinitely. Invalid JSON or a JSON API error can appear as a successful “no results” search. Similar unchecked parsing patterns occur in other clients. The featured, news, on-this-day, and Did-you-know home pipelines now validate their required structures; broader clients and request deadlines remain outstanding.

**Recommended change:** Add a shared transfer timeout and an overall deadline for multi-stage loads. Bound the wait for optional math images and retain fallback content when that budget expires. Validate JSON parse status, API error objects, and required fields before emitting success. Keep valid empty responses distinct from failures, and ensure every request ends with a defined success, failure, or cancellation outcome. Preserve the existing generation checks when adding deadlines or retries.

**Validation:** Cover noncompleting replies, malformed JSON, HTTP-success API errors, missing fields, valid empty results, and retry recovery. Update the tests that currently encode malformed responses as successful emptiness.

## 3. Define image-metadata failure behavior

**Remaining issue:** Metadata-request failure completes successfully with no resolved images and no error or partial-result indication. Unchecked JSON parsing can also silently produce an image-less result.

**Recommended change:** Define whether metadata failure is a recoverable partial result or a user-visible error, and validate the response accordingly. Retain the asynchronous implementation and fake network seam.

**Validation:** Defer the metadata-stage reply specifically, then switch language or supersede the gallery target and verify no obsolete result. Add explicit failure and empty-image cases; current stale-gallery coverage defers the initial page reply rather than metadata completion.

## 4. Put budgets on caches and math-image work

**Observed:** [ArticleState.h](../src/state/ArticleState.h) holds an unbounded `QMap<int, page>` cache. [WikipediaPageClient](../src/wikipedia_page_client.cpp) caches rasterized math images as base64 data URLs and starts a request for every distinct uncached formula in an article. Individual raster dimensions are capped at 4096, but there is no aggregate cache budget or application-level work queue.

**Opportunity:** Long sessions and formula-heavy articles can accumulate HTML and image data. The extent of the memory and responsiveness cost needs measurement; this review did not profile it.

**Recommended change:** Add bounded cache eviction, preferably by estimated bytes as well as entry count. Limit concurrent math-image processing and deduplicate in-flight work across article requests. Measure HTML processing and SVG conversion before deciding whether to move CPU work off the GUI thread.

**Validation:** Browse a large fixture set repeatedly and record memory and load latency. Assert eviction, refetch behavior, and the configured concurrency limit.

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
2. Add deadlines and response validation, define metadata partial-result semantics, and extend deferred-response coverage across the asynchronous chains.
3. Establish CI, consolidate setup/test documentation and runners, and profile caches before selecting budgets.
4. Migrate the independent QML singleton services when changing startup wiring.
