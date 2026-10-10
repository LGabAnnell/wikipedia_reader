# Split GlobalState through a staged migration

## Summary

Replace `GlobalState` with focused state objects, reuse existing feature models, and isolate loading and errors by feature. Keep a temporary forwarding facade during migration, then remove it.

Use domain-specific static instances, as selected. Preserve the current UI, article cache, history behavior, and StackView navigation.

## Target responsibilities and interfaces

| Owner | Responsibility |
|---|---|
| `ArticleState` | Current article, article loading/error, loading by ID/title, cache, history recording; owns its page client |
| `SearchState` | Search results, searching/completion state, search error; owns its search client |
| `SettingsState` | Language and `languageChanged`; defaults to `en` |
| `ImageSelectionState` | Fullscreen image URL/caption; exposes `selectImage(url, description)` |
| Existing `SectionModel` | Per-view section list, loading/error; retains its dedicated page client |
| Existing `ContentDisplayModel` | Per-view active section index, anchor positions, and article-text search |
| Existing `ImageHomeModel` | Gallery data and gallery loading/error; owns a dedicated page client |
| `ClipboardHelper` | Stateless QML-callable clipboard copying |

Register the four shared states and clipboard helper under the existing `wikipedia_qt` URI. Shared states expose guarded `QPointer` static accessors; feature models retain guarded references to their specific dependencies.

## Migration stages

### 1. Extract settings and image selection

- Create application-owned `SettingsState` and `ImageSelectionState`; register them before QML loads in both application startup and the test harness.
- Make the existing `GlobalState` language and image APIs forward to these objects, including change notifications. Keep one storage owner for each value.
- Move language subscriptions in search, home, sections, and gallery models to `SettingsState`.
- Migrate the language selector, fullscreen image view, and image-click handlers to the new APIs. Set both selection values before emitting notifications.
- Remove the unused search, featured, and home clients from `GlobalState`; `HomeModel` continues owning its existing clients.

### 2. Extract article and search behavior

- Move article data, loading/error, title resolution, cache, and history recording into `ArticleState`. Require a valid `HistoryState` dependency at construction.
- Move search execution and result lifecycle into `SearchState`. Keep text input in `SearchBarModel`, which forwards submission and exposes search-state notifications.
- Migrate search and article QML consumers together: search reads `SearchState`; article displays and article-opening actions use `ArticleState`.
- Keep `GlobalState` article/search APIs as forwarding adapters during this stage. Its legacy loading/error properties forward to article state; search consumers must already use their dedicated properties.
- Give `ImageHomeModel` its own client and loading/error properties. Gallery responses and failures must never modify article content, cache, history, or article feedback.
- Move clipboard copying into `ClipboardHelper`.
- Preserve cache hits and history updates. Clear article cache on language changes; initialize every client from current settings.

### 3. Consolidate sections and remove the facade

- Make `SectionModel` the sole section-list owner. Remove duplicate section storage, fetching, and loading/error handling from the facade.
- Add `currentSectionIndex`, defaulting to `-1`, to `ContentDisplayModel`. Pass it into `Section.qml` through an explicit property for delegate highlighting.
- Reset the active index and clear the anchor-position map when article content or section data changes. Clear section loading/error when the article becomes empty.
- Replace all remaining `GlobalState` callers and migrate test setup/reset code.
- Remove `GlobalState`, its registration, static accessor, forwarding APIs, and obsolete tests. Completion requires no source or test references to it.

Compile new backend state classes once in the existing shared core. Keep view models in their current QML modules; do not introduce core dependencies on those modules or Qt Quick.

## Validation

- Split existing state tests by responsibility; verify initial values, notifications, cache hits, history updates, and language-driven cache invalidation.
- Preserve search tests for blank input, deferred requests, empty success, failure, and retry.
- Add deferred-request checks proving search completion/errors cannot change article loading/errors, and section/gallery failures cannot affect article state.
- Preserve section scrolling, highlighting, resizing, gallery captions, inline image selection, and back-navigation tests.
- Verify singleton teardown/recreation clears static pointers and startup remains free of QML warnings and unexpected requests.
- Build with testing enabled and run the full CTest suite headlessly after each stage.

## Assumptions and boundaries

- One application-owned instance per shared state; construct dependencies before feature models and destroy them afterward.
- The facade is temporary and removed in stage 3.
- Preserve current language-change refresh behavior; this refactor adds no automatic reloads.
- Network timeouts, retries, general request cancellation, HTML processing, cache limits, and history-schema changes remain separate work.
