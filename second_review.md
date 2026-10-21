# Source review findings

Review scope: `src/` under the repository's current working tree. The findings below were reproduced with temporary fixture tests; no application source files were changed during the review.

## Findings

### [P2] Article API errors can be accepted as successful empty articles

The parse response handler checks only the network error. A successful HTTP response containing a MediaWiki API error or malformed JSON is treated as an empty HTML body, processed, then emitted as `pageReceived`. `ArticleState` can cache and add this invalid page to history as a successful visit.

Relevant code: [src/wikipedia_page_client.cpp](src/wikipedia_page_client.cpp#L401)

### [P2] Gallery uses the global article instead of the visible article

After opening article B from article A and navigating Back to A, the visible article view is A while `ArticleState` still holds B. `ImageHomeModel::loadImagesForCurrentPage()` reads the singleton's page ID, so the gallery displays B's images under A's title. A temporary navigation fixture reproduced this mismatch.

Relevant code: [src/modules/ImageDisplayModule/ImageHomeModel.cpp](src/modules/ImageDisplayModule/ImageHomeModel.cpp#L60)

### [P2] Failed article selections leave the previous article visible

`loadArticleByPageId()` does not clear the current page before starting an uncached request. If that request fails, the old article remains in `ArticleState`; the new article view can display stale content together with the new error. A temporary fixture reproduced this behavior.

Relevant code: [src/state/ArticleState.cpp](src/state/ArticleState.cpp#L55)

### [P2] Search API errors are shown as “No results found”

The search reply handler treats every successful HTTP response as a valid search result. A MediaWiki API error object or malformed response parses to an empty result list and emits `searchCompleted`, so the UI reports no results instead of surfacing an error. A temporary fixture reproduced this behavior.

Relevant code: [src/wikipedia_search_client.cpp](src/wikipedia_search_client.cpp#L37)

## Validation

- `./scripts/run_tests.sh`: build succeeded; all 27 registered CTest entries passed.
- Temporary fixture checks reproduced all five findings. The checks were kept under `/tmp` and were not added to the repository.
- Live Wikipedia API behavior was not tested.
