# Improvement opportunities

Reviewed: 2026-10-16. Source baseline: `e3a184e` (including language-transition implementation `c76d821`; previous review: `45c773a`).

This report is based on static inspection of the current application, build configuration, and tests. No application code was changed, and no build, runtime reproduction, or test suite was run for this review. Findings below distinguish directly observed implementation problems from proposed hardening work. File links are relative to this document; symbol names identify the relevant code.

## Implementation status and remaining priorities

| Priority | Opportunity | Status | Main benefit |
| --- | --- | --- | --- |
| Completed | Isolate language changes and stale replies | Implemented; broader chain coverage remains | Prevent articles from the wrong Wikipedia edition |
| High | Make history identities language-aware | Open | Open the correct article from history |
| High | Enforce persistent history limits and write consistency | Open | Bound storage and avoid misleading history state |
| High | Add request deadlines and validate API responses | Open; explicit cancellation is implemented | Recover from stalled or invalid responses |
| Low | Finish image metadata failure semantics and regression coverage | Nested event loop removed; follow-up remains | Make partial results predictable |
| Medium | Complete home-screen feedback and featured handling | Partial: refresh and title/URL fixes implemented | Make failures visible and eliminate wasted requests |
| Medium | Bound article and math-image caches | Open | Control memory usage during long sessions |
| Medium | Automate regression checks in CI | Open | Catch failures before merging |
| Low | Refresh developer documentation and test tooling | Partial: current coverage guide and incremental runner exist | Make setup reproducible and preserve build output |

## 1. Treat language changes as complete state transitions — implemented

**Implemented:** [SettingsState.cpp](../src/state/SettingsState.cpp) emits `languageTransitionStarted()` before `languageChanged()`. Consumers synchronously cancel pending work and clear edition-bound state, then adopt the new language. [ArticleState.cpp](../src/state/ArticleState.cpp) clears the current article, cache, loading state, and errors. [SearchState.cpp](../src/state/SearchState.cpp) clears results/feedback and resubmits stored nonblank search text once. [HomeModel.cpp](../src/modules/HomeModule/HomeModel.cpp) clears its cards and featured properties and refreshes all feeds. Sections, gallery data, and selected images also reset.

All four Wikipedia clients use [WikipediaRequestTracker](../src/wikipedia_request_context.h) to capture language and generation, invalidate requests before aborting replies, and reject obsolete results/errors and chained work. Article selections invalidate previous work before cache lookup; gallery/section target changes and home refreshes also cancel prior requests. Chained article HTML, math-image, metadata, and random-summary requests carry the captured context.

**Chosen view behavior:** The navigation stack stays in place. [ContentDisplay.qml](../src/modules/ContentDisplayModule/ContentDisplay.qml) clears visible and covered article snapshots on a language transition, so Back does not reveal old-edition content. Browsing history is explicitly outside this implementation and remains unsafe across editions; see section 2.

**Existing regression coverage:** [WikipediaRequestIsolationTest.cpp](../tests/WikipediaRequestIsolationTest.cpp) checks obsolete search successes/errors, featured replies, and page/random-title follow-up suppression. [ArticleStateTest.cpp](../tests/ArticleStateTest.cpp) checks content/cache clearing, obsolete article/title replies, and cached selections superseding pending work. [SearchStateTest.cpp](../tests/SearchStateTest.cpp) covers search restart and rapid language changes. QML tests cover clearing covered article views without changing stack depth, clearing feature state and rejecting an obsolete gallery reply, and home refresh on language changes in [tst_article.qml](../tests/qml/tst_article.qml), [tst_feature_isolation.qml](../tests/qml/tst_feature_isolation.qml), and [tst_navigation.qml](../tests/qml/tst_navigation.qml).

**Remaining validation:** Extend deferred-response coverage across each HTML/math-image/metadata stage and overlapping uncached article selections. Existing tests cover selected paths, not every chain. [The language isolation document](language-change-state-isolation-plan.md) now describes the implemented states and behavior; the old content-preservation test has been replaced by `languageChangeClearsCurrentArticleAndCache`.

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

**Observed:** [The shared network manager](../src/wikipedia_network_access_manager.h) applies an HTTP/2 compatibility policy but no request timeout. Clients now expose explicit cancellation and reject obsolete generations (section 1), but there is still no deadline for an active request. Article completion waits for all requested math images; completed math-image failures retain the original source as a fallback, but a stalled reply can still hold up the article.

[WikipediaSearchClient.cpp](../src/wikipedia_search_client.cpp) emits `searchCompleted()` on transport success even when JSON is malformed or the expected response structure is missing. `parseSearchResults()` converts those cases into an empty vector. [WikipediaSearchClientTest.cpp](../tests/WikipediaSearchClientTest.cpp) explicitly expects zero results for malformed JSON and `{}`.

**Impact:** A stalled request can keep controls busy indefinitely. Invalid JSON or a JSON API error can appear as a successful “no results” search. Similar unchecked parsing patterns occur in the page and home clients and should be audited.

**Recommended change:** Add a shared transfer timeout and an overall deadline for multi-stage loads. Bound the wait for optional math images and retain fallback content when that budget expires. Validate JSON parse status, API error objects, and required fields before emitting success. Keep valid empty responses distinct from failures, and ensure every request ends with a defined success, failure, or cancellation outcome. Preserve the implemented generation checks when adding deadlines or retries.

**Validation:** Cover noncompleting replies, malformed JSON, HTTP-success API errors, missing fields, valid empty results, and retry recovery. Update the tests that currently encode malformed responses as successful emptiness.

## 5. Asynchronous image metadata lookup — implemented, with follow-up work

**Implemented:** `WikipediaPageClient::fetchImageUrlsFromTitles()` in [wikipedia_page_client.cpp](../src/wikipedia_page_client.cpp) now takes owned page data, captured request context, and a completion callback. It completes from `QNetworkReply::finished`; the local `QEventLoop::exec()` and output references are gone. It checks the generation before starting work and before completion, and constructs the URL from the captured language.

**Remaining issue:** Metadata-request failure still completes successfully with no resolved images and no error or partial-result indication. The same unchecked JSON parsing can silently produce an image-less result.

**Recommended change:** Define whether metadata failure is a recoverable partial result or a user-visible error, and validate the response accordingly. Retain the asynchronous implementation and fake network seam.

**Remaining validation:** Defer the metadata-stage reply specifically, then switch language or supersede the gallery target and verify no obsolete result. Add explicit failure and empty-image cases; current stale-gallery coverage defers the initial page reply rather than metadata completion.

## 6. Complete the home experience and featured-article pipeline

**Observed:** [HomeScreen.qml](../src/modules/HomeModule/HomeScreen.qml) renders news, on-this-day, and did-you-know sections, while `HomeModel::fetchHomeData()` also fetches a featured article and its images. The featured properties have no consumer in that screen. `HomeModel` exposes no loading/error status; only the featured client's error is connected to `handleError()`, which logs a warning. Errors from its home and page clients are not connected there.

**Implemented:** Language transitions now clear home data and refresh all feeds, and a refresh cancels the previous generation. `handleFeaturedArticleReceived()` preserves the display title, builds the slug from a separate local value, and percent-encodes it with `QUrl::toPercentEncoding()`. The previous title mutation/comparison defect is fixed.

**Recommended change:** Decide whether featured content belongs on the home screen: render it or remove the unused fetch chain. Expose loading, empty, error, and retry state per section so one failed feed does not obscure successful sections. Connect all relevant errors.

**Validation:** Add a dedicated `tst_home.qml` using fixtures for rendered feeds, partial failures, empty data, and retry. Add regression coverage for the fixed featured title/URL behavior using spaces and URL-sensitive characters. `tst_navigation.qml::test_language_change_keeps_home_and_refreshes_its_data` already covers clearing/refilling news on a language change while retaining the home root; there is still no dedicated home test file.

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

**Implemented:** [full_testing.md](full_testing.md) now documents the current dependencies, separate state classes, shared backend, fixture controls, 25 registered CTest entries, and coverage limits. [scripts/run_tests.sh](../scripts/run_tests.sh) preserves existing build output by default, configures when needed, supports an explicit clean rebuild and test filter, sets the offscreen platform, and uses `set -euo pipefail` for immediate failure propagation.

**Remaining issues:** No tracked README is present. `AGENTS.md`, [TESTING.md](../TESTING.md), and [improvement-observations.md](improvement-observations.md) still describe `GlobalState` and/or previously duplicated backend compilation. Testing guidance remains split across those files and `docs/`. The build requires Lexbor 3.0, absent from the agent guide's dependency overview. The legacy [test.sh](../test.sh) still deletes `build`, rebuilds with `-j14`, and lacks fail-fast behavior; the agent guide incorrectly describes it as only running CTest. The incremental runner also hardcodes `-j14`.

**Recommended change:** Add a concise README with actual dependencies and build/run/test instructions. Consolidate authoritative testing guidance around the current coverage guide, update the agent guide, and label older proposals with implementation status. Replace or delegate the legacy `test.sh` to the incremental runner and make build parallelism configurable. CMake presets could provide shared debug/test configurations.

**Remaining validation:** Follow the documented steps from a fresh checkout. Exercise the incremental runner's configuration/build failure paths and confirm failure propagation; verify a unified root test command preserves existing build output by default.

## Suggested implementation sequence

1. Fix persistent history trimming and add its state-level regression test.
2. Implement language-aware history identity with a schema migration and cross-edition/restart tests, building on the completed language-transition isolation.
3. Add deadlines and response validation, define metadata partial-result semantics, and extend deferred-response coverage across the asynchronous chains.
4. Complete home-screen feedback and decide whether to render featured content; add dedicated coverage, including regression tests for the completed title/URL fix.
5. Establish CI, consolidate setup/test documentation and runners, and profile caches before selecting budgets.

The shared backend, separated state classes, language-transition/request isolation, asynchronous metadata lookup, featured-title fix, and incremental test runner are implemented foundations. Preserve those improvements; they should no longer be treated as outstanding implementation work. Test coverage cited here was inspected in source, not executed during this review.
