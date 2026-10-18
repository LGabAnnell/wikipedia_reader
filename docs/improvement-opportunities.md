# Improvement opportunities

Reviewed: 2026-10-18. Source baseline: `af85b44` (previous status review: `03ad867`; includes language-transition implementation `c76d821`).

This report is based on static inspection of the current application, build configuration, and tests. No application code was changed, and no build, runtime reproduction, or test suite was run for this review. Findings below distinguish directly observed implementation problems from proposed hardening work. File links are relative to this document; symbol names identify the relevant code.

History implementation follow-up (2026-10-18): language-aware history is now implemented and validated with a Debug build and the full offscreen CTest suite (25/25 passing). The original review context above and unrelated findings remain unchanged.

## Implementation status and remaining priorities

| Priority | Opportunity | Status | Main benefit |
| --- | --- | --- | --- |
| Completed | Isolate language changes and stale replies | Implemented; broader chain coverage remains | Prevent articles from the wrong Wikipedia edition |
| Completed | Replace nested image-metadata event loop | Asynchronous callback implemented; failure semantics remain below | Avoid reentrant blocking during gallery loads |
| Completed | Identify requests to Wikimedia | Shared User-Agent for API requests and QML images | Identify the app, version, and contact URL |
| Completed | Add basic setup and distribution documentation | README and local Flatpak packaging implemented; test docs still need consolidation | Document dependencies and support sandboxed distribution |
| Completed | Make history identities language-aware | Implemented with schema migration and cross-edition regression coverage | Open the correct article from history |
| High | Enforce persistent history limits and write consistency | Open | Bound storage and avoid misleading history state |
| High | Add request deadlines and validate API responses | Open; explicit cancellation is implemented | Recover from stalled or invalid responses |
| Low | Finish image metadata failure semantics and regression coverage | Nested event loop removed; follow-up remains | Make partial results predictable |
| Medium | Complete home-screen feedback and featured handling | Partial: refresh, title/URL fixes, news fallback, and rendered-news coverage implemented | Make failures visible and eliminate wasted requests |
| Medium | Bound article and math-image caches | Open | Control memory usage during long sessions |
| Medium | Automate regression checks in CI | Open | Catch failures before merging |
| Low | Consolidate developer documentation and test tooling | Partial: README, current coverage guide, and incremental runner exist | Make setup reproducible and preserve build output |

### Implemented since the previous status review

- **News fallback and feed completeness:** [WikipediaHomeClient](../src/wikipedia_home_client.cpp) prefers the full most-read list, looks back at most two days when today's list is unavailable, and retains today's linked news stories when no recent most-read list is found. Stories are parsed without requiring `mostread`; their HTML descriptions become plain text. Fixture tests cover fallback, failures, cancellation, and rendering a 43-item list; see section 6.
- **Wikimedia request identification:** [The shared network manager](../src/wikipedia_network_access_manager.h) adds `WikipediaReader/<version>`, the project contact URL, and the Qt version to the User-Agent for both API clients and QML images (`fcd585f`). This does not add deadlines or response validation.
- **Setup and packaging:** [README.md](../README.md) now lists dependencies and build/run commands. [The Flatpak manifest](../io.github.lgabannell.wikipedia_reader.yml), [installation rules](../packaging/Flatpak.cmake), desktop/AppStream metadata, license, [builder helper](../scripts/flatpak-builder.sh), and [packaging guide](../packaging/README.md) provide a local distribution workflow (`8e072a6`). Packaging is implemented in source; it was not built or runtime-validated in this review.
- **Search entry and documentation cleanup:** [SearchScreen.qml](../src/SearchScreen.qml) focuses the search input when its stack item activates (`af85b44`). Obsolete `god-object-refactor.md` and `improvement-observations.md` planning documents were removed (`1a9f15d`).

The shared core library, separated state classes, language isolation, asynchronous metadata lookup, featured-title fix, and incremental test runner were already implemented at the previous review. The sections below preserve their remaining follow-ups without treating the completed foundations as open work.

## 1. Treat language changes as complete state transitions — implemented

**Implemented:** [SettingsState.cpp](../src/state/SettingsState.cpp) emits `languageTransitionStarted()` before `languageChanged()`. Consumers synchronously cancel pending work and clear edition-bound state, then adopt the new language. [ArticleState.cpp](../src/state/ArticleState.cpp) clears the current article, cache, loading state, and errors. [SearchState.cpp](../src/state/SearchState.cpp) clears results/feedback and resubmits stored nonblank search text once. [HomeModel.cpp](../src/modules/HomeModule/HomeModel.cpp) clears its cards and featured properties and refreshes all feeds. Sections, gallery data, and selected images also reset.

All four Wikipedia clients use [WikipediaRequestTracker](../src/wikipedia_request_context.h) to capture language and generation, invalidate requests before aborting replies, and reject obsolete results/errors and chained work. Article selections invalidate previous work before cache lookup; gallery/section target changes and home refreshes also cancel prior requests. Chained article HTML, math-image, metadata, and random-summary requests carry the captured context.

**Chosen view behavior:** The navigation stack stays in place. [ContentDisplay.qml](../src/modules/ContentDisplayModule/ContentDisplay.qml) clears visible and covered article snapshots on a language transition, so Back does not reveal old-edition content. Browsing history now retains and restores its saved edition through the follow-up in section 2.

**Existing regression coverage:** [WikipediaRequestIsolationTest.cpp](../tests/WikipediaRequestIsolationTest.cpp) checks obsolete search successes/errors, featured replies, and page/random-title follow-up suppression. [ArticleStateTest.cpp](../tests/ArticleStateTest.cpp) checks content/cache clearing, obsolete article/title replies, and cached selections superseding pending work. [SearchStateTest.cpp](../tests/SearchStateTest.cpp) covers search restart and rapid language changes. QML tests cover clearing covered article views without changing stack depth, clearing feature state and rejecting an obsolete gallery reply, and home refresh on language changes in [tst_article.qml](../tests/qml/tst_article.qml), [tst_feature_isolation.qml](../tests/qml/tst_feature_isolation.qml), and [tst_navigation.qml](../tests/qml/tst_navigation.qml).

**Remaining validation:** Extend deferred-response coverage across each HTML/math-image/metadata stage and overlapping uncached article selections. Existing tests cover selected paths, not every chain. [The language isolation document](language-change-state-isolation-plan.md) now describes the implemented states and behavior; the old content-preservation test has been replaced by `languageChangeClearsCurrentArticleAndCache`.

## 2. Include language in browsing-history identity — implemented

**Implemented:** [HistoryState.cpp](../src/state/HistoryState.cpp) and [HistoryDatabase.cpp](../src/state/db/HistoryDatabase.cpp) identify articles by `(language, page_id)`. New visits require a nonblank edition; revisits update only the matching edition. A transactional schema-version-1 migration adds a nullable language column and replaces the page-ID-only index with a unique composite index. Existing IDs, titles, and timestamps survive with SQL `NULL` for the unknown edition. Migration failures roll back the schema and version, report an error, and block database operations until initialization succeeds.

**Chosen behavior:** [History.qml](../src/modules/HistoryModule/History.qml) displays a combined list with edition labels. Opening a known entry restores its language before loading the saved page ID; the header language selector follows that change. Unknown legacy entries remain visible as “Unknown edition” with opening disabled. “Clear History” clears every edition and legacy entry. The existing combined 50-item in-memory limit remains; persistent retention and general write consistency are separate work in section 3.

**Validation:** [HistoryDatabaseTest.cpp](../tests/HistoryDatabaseTest.cpp) covers legacy migration, repeat initialization, rollback and blocked operations, cross-edition persistence/deduplication, composite uniqueness, and clearing known plus unknown editions. [ArticleStateTest.cpp](../tests/ArticleStateTest.cpp) covers edition recording, cache visits, state recreation, invalid history destinations, restored-edition failures, completion observers, and rejected obsolete replies. [tst_history.qml](../tests/qml/tst_history.qml) and [tst_navigation.qml](../tests/qml/tst_navigation.qml) cover disabled legacy entries, edition labels, cross-edition requests and rendered content, selector synchronization, cached list reordering, Back navigation, and clearing the combined list. Debug build and full offscreen CTest suite passed (25/25).

## 3. Fix history retention and database failure semantics

**Confirmed control-flow defect:** In [HistoryState.cpp](../src/state/HistoryState.cpp), `addToHistory()` removes the last in-memory item when the size exceeds `MAX_HISTORY_ITEMS`, then tests that same size again before calling `removeOldestItems()`. The second condition is false after trimming. Consequently, normal additions do not enforce the database limit.

**Related issue:** Both adding and clearing history mutate memory before checking database success. A failed clear can make the UI appear empty while the records remain on disk and return after restart. Database initialization also runs inside the `HistoryDatabase` constructor, before `HistoryState` connects its forwarding signals, so those initial notifications cannot be forwarded through those connections.

**Recommended change:** Make insertion and retention pruning one transaction. Publish the resulting in-memory state only after successful persistence, or explicitly expose an unsaved state. Connect initialization observers before initialization, or expose initialization status and error properties that callers can inspect afterward.

**Validation:** Add more than 50 distinct items through `HistoryState`, inspect the stored row count, and reload. Inject insertion, clearing, and initialization failures and verify visible state and error reporting. Existing database-helper tests do not replace a state-level retention regression test.

## 4. Bound requests and distinguish invalid responses from empty results

**Observed:** [The shared network manager](../src/wikipedia_network_access_manager.h) applies the app User-Agent and an HTTP/2 compatibility policy but no request timeout. Clients expose explicit cancellation and reject obsolete generations (section 1), but there is still no deadline for an active request. Article completion waits for all requested math images; completed math-image failures retain the original source as a fallback, but a stalled reply can still hold up the article. The news lookback is bounded by request count, not elapsed time.

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

**Implemented:** Language transitions clear home data and refresh all feeds, and a refresh cancels the previous generation. `handleFeaturedArticleReceived()` preserves the display title, builds the slug from a separate local value, and percent-encodes it with `QUrl::toPercentEncoding()`. The previous title mutation/comparison defect is fixed.

[WikipediaHomeClient.cpp](../src/wikipedia_home_client.cpp) now handles feeds with news stories but no most-read articles. `parseNewsItems()` prefers a nonempty `mostread.articles` array and otherwise uses each news story's first linked article, normalizes its title, and converts the story HTML to plain text. `getNewsItems()` looks back at most two days for a nonempty most-read list and preserves the full list without truncation. If historical feeds are empty or fail, it retains today's news; a failure of today's request still emits an error. These follow-up requests retain the captured language/generation and stop after cancellation or a language change. [NewsSection.qml](../src/modules/HomeModule/NewsSection.qml) exposes the rendered items for QML assertions, and [HomeScreen.qml](../src/modules/HomeModule/HomeScreen.qml) lets the content scroll.

**Recommended change:** Decide whether featured content belongs on the home screen: render it or remove the unused fetch chain. Expose loading, empty, error, and retry state per section so one failed feed does not obscure successful sections. Connect all relevant errors.

**Existing regression coverage:** [WikipediaHomeClientTest.cpp](../tests/WikipediaHomeClientTest.cpp) now uses the fake transport to cover current most-read data, previous-day fallback, empty/404/503 historical feeds, the two-day bound, retaining today's news, current-feed errors, and obsolete lookback replies after cancellation or a language change. Parser cases cover most-read precedence, missing/empty most-read arrays, normalized titles, story text, and missing links. [tst_navigation.qml](../tests/qml/tst_navigation.qml) checks a 43-item rendered news list, nonzero final-item height, scrollable content, and switching between English most-read data and French news fallback while retaining the home root.

**Remaining validation:** Add dedicated home integration coverage for pending/empty/error/retry feedback and partial failures across sections, plus featured title/URL regression cases with spaces and URL-sensitive characters. There is still no `tst_home.qml`, but news rendering and fallback are already covered in the navigation tests; a new filename is not itself the missing behavior.

## 7. Put budgets on caches and math-image work

**Observed:** [ArticleState.h](../src/state/ArticleState.h) holds an unbounded `QMap<int, page>` cache. [WikipediaPageClient](../src/wikipedia_page_client.cpp) caches rasterized math images as base64 data URLs and starts a request for every distinct uncached formula in an article. Individual raster dimensions are capped at 4096, but there is no aggregate cache budget or application-level work queue.

**Opportunity:** Long sessions and formula-heavy articles can accumulate HTML and image data. The extent of the memory and responsiveness cost needs measurement; this review did not profile it.

**Recommended change:** Add bounded cache eviction, preferably by estimated bytes as well as entry count. Limit concurrent math-image processing and deduplicate in-flight work across article requests. Measure HTML processing and SVG conversion before deciding whether to move CPU work off the GUI thread.

**Validation:** Browse a large fixture set repeatedly and record memory and load latency. Assert eviction, refetch behavior, and the configured concurrency limit.

## 8. Add reproducible CI around the existing tests

**Observed:** No tracked `.github/workflows` configuration is present. [tests/CMakeLists.txt](../tests/CMakeLists.txt) already registers backend and QML tests, and the fixture transport provides a foundation for deterministic network behavior. This observation does not rule out externally configured CI.

**Recommended change:** Add a Linux job that provisions Qt, tinyxml2, and Lexbor, configures with `BUILD_TESTING=ON`, builds, and runs `ctest --test-dir build --output-on-failure` with an offscreen platform available. Account for GUI-backed C++ tests as well as QML tests. Keep live-network tests separate and opt-in. Upload failure logs; consider relocated-install and Flatpak smoke tests because deployment rules, a shared core library, and local Flatpak packaging are already present. Packaging support does not establish automated CI coverage.

**Validation:** Demonstrate a clean checkout build and a failing-test run that fails the job and preserves useful diagnostics. Document the supported dependency baseline, then expand platform coverage as needed.

## 9. Refresh documentation and make test execution less disruptive

**Implemented:** [README.md](../README.md) now provides the application overview, actual dependency baseline (including Lexbor 3.0+), build/run commands, license, and links to testing and packaging. [packaging/README.md](../packaging/README.md) documents the local Flatpak workflow. [full_testing.md](full_testing.md) documents the separate state classes, shared backend, fixture controls, 25 registered CTest entries, and coverage limits. [scripts/run_tests.sh](../scripts/run_tests.sh) preserves existing build output by default, configures when needed, supports an explicit clean rebuild and test filter, sets the offscreen platform, and uses `set -euo pipefail` for immediate failure propagation. Obsolete refactor/observation planning documents have been removed.

**Remaining issues:** [AGENTS.md](../AGENTS.md) and [TESTING.md](../TESTING.md) still describe `GlobalState` and/or previously duplicated backend compilation. The README links to that outdated testing guide, and [full_testing.md](full_testing.md) has not yet incorporated the new news fallback/rendering coverage. Testing guidance remains split across those files and `docs/`. The build requires Lexbor 3.0, absent from the agent guide's dependency overview; the guide also incorrectly says no README exists. The legacy [test.sh](../test.sh) still deletes `build`, rebuilds with `-j14`, and lacks fail-fast behavior; the agent guide incorrectly describes it as only running CTest. The incremental runner also hardcodes `-j14`.

**Recommended change:** Keep the existing README and packaging guide; consolidate authoritative testing guidance around the coverage guide, refresh its news coverage, update the agent guide, and label remaining older proposals with implementation status. Replace or delegate the legacy `test.sh` to the incremental runner and make build parallelism configurable. CMake presets could provide shared debug/test configurations.

**Remaining validation:** Follow the documented steps from a fresh checkout. Exercise the incremental runner's configuration/build failure paths and confirm failure propagation; verify a unified root test command preserves existing build output by default.

## Suggested implementation sequence

1. Fix persistent history trimming and add its state-level regression test.
2. Add deadlines and response validation, define metadata partial-result semantics, and extend deferred-response coverage across the asynchronous chains.
3. Complete home-screen feedback and decide whether to render featured content; extend the existing news coverage to section failures/retry and featured title/URL behavior.
4. Establish CI, consolidate the existing setup/test documentation and runners, and profile caches before selecting budgets.

The shared backend, separated state classes, language-transition/request isolation, language-aware history identity and migration, asynchronous metadata lookup, featured-title fix, bounded news lookback and fallback, request identification, README, local Flatpak packaging, search-entry focus, and incremental test runner are implemented foundations. Remaining priorities are persistent history retention/write consistency, request deadlines/response validation, metadata failure semantics, home feedback/featured handling, cache budgets, CI, and documentation/test-runner consolidation. Test coverage cited in the original review was inspected in source; the history implementation follow-up above was built and tested.
