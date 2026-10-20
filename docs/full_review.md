# Full Code Review: `src/`

Review of all 82 source files under `src/` (C++20 / Qt6 / QML).

## Critical

None found. No XSS vectors, secret leakage, data loss, or crash-on-launch bugs.

## High

### Resolved — `src/modules/ImageDisplayModule/ImageGallery.qml` — Back button referenced external `stackView`

The Back handler now uses `root.StackView.view` and pops only when the owning
stack exists and has more than one item. The button exposes the stable
`imageGalleryBackButton` selector. Regression tests cover returning to the
previous item in a separately named stack, retaining a root-level gallery, and
safely clicking Back on a standalone gallery, with no QML warnings.
Validation: `./scripts/run_tests.sh -R QmlTests.tst_images` passed.

### Resolved — `src/state/NavigationState.cpp` — `navigateToView()` threw for unregistered view names

`navigateToView()` now uses `views.find(view)`, logs a warning containing an
unregistered name, and returns without emitting `pushView`. QML regression tests
cover empty and unknown names, unchanged stack depth/current item, and successful
registered-view navigation afterward.
Validation: `./scripts/run_tests.sh -R '^QmlTests\.tst_navigation$'` passed.

### `src/state/NavigationState.cpp:35` — Same `views.at(view)` pattern in `setCurrentView()`

Same crash risk as above. This slot is currently dead code (never called from QML), but the pattern is unsafe.

### `src/state/NavigationState.h:51` — Missing `#include <map>` for `std::map`

The header declares `std::map<QString, QQmlComponent *> views;` without including `<map>`. It compiles only via transitive includes — not portable.

## Medium

### `src/wikipedia_page_client.cpp:172` — Dead `imageTitles` local in `getPageById()`

The method fetches `prop=extracts|images` with `imlimit=50`, populates a local `QStringList imageTitles` from the response (lines 180–187), then calls `fetchPageContentWithImages()` without passing it. The `extract` from this response is also overwritten later by `fetchPageContentWithImages`. The `images` prop and `imageTitles` extraction are entirely wasted — remove `prop=extracts|images` and `imlimit=50` from the query, and delete the dead code. The gallery loads images separately via `getPageWithImages()`, so this is purely wasteful.

### `src/wikipedia_home_client.cpp:33` — `getOnThisDayEvents(int month, int day)` ignores its parameters

The function signature takes `month` and `day` but uses `QDate::currentDate()` internally (lines 34, 38–39). The caller (`HomeModel::loadSection`) passes today's date, so it works by coincidence. Either use the parameters or remove them.

### `src/wikipedia_featured_client.cpp:37` — Double `reply->deleteLater()`

`getFeaturedArticleOfTheDay()` calls `reply->deleteLater()` in its lambda (line 37), and `onFeaturedArticleReply()` calls it again (line 57). Qt handles double-`deleteLater()` gracefully, but one of the two calls should be removed.

### `src/wikipedia_models.h:108` — `section` struct: `level` and `index` lack default initialization

```cpp
int level;    // uninitialized
int index;    // uninitialized
```

Every other numeric member in the file has `= 0`. Default-constructed `section` objects have indeterminate values. Add `= 0` (or `= -1` for `index`) to match the pattern used elsewhere.

### `src/Main.qml:143` — Dead `replaceView` signal connection

`NavigationState.replaceView.connect(...)` calls `stackView.replace(null, newView)`, but `replaceView` is only emitted by `setCurrentView()`, which is never called from QML. This is dead code that also violates the AGENTS.md rule: "Use `StackView.push()`/`pop()` — NOT `replace` — to preserve history." Remove the signal, slot, and connection.

### `src/modules/ContentDisplayModule/CMakeLists.txt:1` — Wrong file comment

Line 1 says `# HistoryModule CMakeLists.txt` — this is the ContentDisplayModule.

### `src/modules/HeaderModule/CMakeLists.txt:1` — Wrong file comment

Line 1 says `# HistoryModule CMakeLists.txt` — this is the HeaderModule.

## Low

### `src/wikipedia_page_client.cpp:24` — `stripHtml()` handles only 6 HTML entities

`&amp;`, `&lt;`, `&gt;`, `&quot;`, `&#39;`, `&nbsp;` are decoded. Numeric entities (`&#x27;`, `&#169;`), `&apos;`, and others are left as-is. For Wikipedia image descriptions this is usually sufficient, but a `QTextDocument`-based approach would be more complete.

### `src/wikipedia_page_client.cpp:253` — `parsePageWithImages()` stores image titles in the `imageUrls` field

The comment says "caller fetches URLs" — the caller (`onPageWithImagesReply`) extracts titles from `imageUrls`, clears it, then calls `fetchImageUrlsFromTitles()`. This overloading of `imageUrls` is fragile; a dedicated `imageTitles` field would be clearer (the field already exists on the `page` struct).

### `src/modules/HeaderModule/HeaderModel.cpp:18` — `renderSvgToImage()` reads the SVG file twice

`calculateSvgScaleFactor()` opens and reads the file (lines 20–29), then `renderSvgToImage()` opens and reads the same file again (lines 55–63). Pass the loaded `QByteArray` or `QSvgRenderer` between the two calls.

### `src/SearchScreen.qml:5` — Inconsistent versioned imports

Uses `import wikipedia_qt 1.0` while most other files use unversioned `import wikipedia_qt`. AGENTS.md notes versioned imports appear in "older files" — worth normalizing.

## Summary

| Severity | Unresolved count |
|----------|-------|
| Critical | 0        |
| High     | 2        |
| Medium   | 8        |
| Low      | 4        |

The image gallery Back button and invalid `navigateToView()` findings are resolved
and covered by regression tests. The remaining `views.at()` crash risk in
`setCurrentView()` and the missing `<map>` include are quick, safe fixes. The dead
code in `getPageById()` wastes API bandwidth on every article load.
