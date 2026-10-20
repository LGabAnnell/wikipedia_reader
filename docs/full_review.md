# Full Code Review: `src/`

Source review of `src/` (C++20 / Qt6 / QML). Findings reconciled with
commit `85752d5` on 2026-10-20. This update checks the existing findings and
recent regression coverage; it is not a fresh audit of every source file.
The on-this-day fix and its focused validation were added on 2026-10-20
after that reconciliation.
See [the testing guide](full_testing.md) for the current coverage inventory.

## Critical

No critical findings recorded in this review.

## High

### Resolved — `src/modules/ImageDisplayModule/ImageGallery.qml` — Back button referenced external `stackView`

The Back handler now uses `root.StackView.view` and pops only when the owning
stack exists and has more than one item. The button exposes the stable
`imageGalleryBackButton` selector. Regression tests cover returning to the
previous item in a separately named stack, retaining a root-level gallery, and
safely clicking Back on a standalone gallery, with no QML warnings.
Previously recorded validation: `./scripts/run_tests.sh -R QmlTests.tst_images` passed.

### Resolved — `src/state/NavigationState.cpp` — `navigateToView()` threw for unregistered view names

`navigateToView()` now uses `views.find(view)`, logs a warning containing an
unregistered name, and returns without emitting `pushView`. QML regression tests
cover empty and unknown names, unchanged stack depth/current item, and successful
registered-view navigation afterward.
Previously recorded validation: `./scripts/run_tests.sh -R '^QmlTests\.tst_navigation$'` passed.

### `src/state/NavigationState.cpp:19` — Same `views.at(view)` pattern in `setCurrentView()`

`setCurrentView()` still uses `views.at(view)` and can throw for an unregistered
name. The guard added to `navigateToView()` does not protect this slot. No current
QML caller uses it; remove the unused replacement path or guard its lookup too.
The new invalid-name regression exercises `navigateToView()` only.

### `src/state/NavigationState.h:51` — Missing `#include <map>` for `std::map`

The header declares `std::map<QString, QQmlComponent *> views;` without including `<map>`. It compiles only via transitive includes — not portable.

## Medium

### `src/wikipedia_page_client.cpp:172` — Dead `imageTitles` local in `getPageById()`

The method fetches `prop=extracts|images` with `imlimit=50`, populates a local `QStringList imageTitles` from the response (lines 172–187), then calls `fetchPageContentWithImages()` without passing it. The `extract` from this response is also overwritten later by `fetchPageContentWithImages`. The `images` prop and `imageTitles` extraction are entirely wasted — remove `prop=extracts|images` and `imlimit=50` from the query, and delete the dead code. The gallery loads images separately via `getPageWithImages()`, so this is purely wasteful.

### Resolved — `src/wikipedia_home_client.cpp:33` — `getOnThisDayEvents(int month, int day)` ignored its parameters

The request URL now uses the supplied `month` and `day` instead of
`QDate::currentDate()`, retaining two-digit formatting for both path segments.
`testOnThisDayUsesRequestedDate` in `tests/WikipediaHomeClientTest.cpp` covers
January 2 in English and December 31 in French using fixture responses. Each
case verifies one request, successful completion, and no error signal.
The `WikipediaHomeClientTest` target was rebuilt successfully. Validation:
`ctest --test-dir build -R '^WikipediaHomeClientTest$' --output-on-failure` passed.

### `src/wikipedia_featured_client.cpp:27` — Double `reply->deleteLater()`

`getFeaturedArticleOfTheDay()` calls `reply->deleteLater()` in its lambda (line 27), and `onFeaturedArticleReply()` calls it again (line 54). Qt handles double-`deleteLater()` gracefully, but one of the two calls should be removed.

### `src/wikipedia_models.h:128` — `section` struct: `level` and `index` lack default initialization

```cpp
int level;    // uninitialized
int index;    // uninitialized
```

Every other numeric member in the file has `= 0`. Default-constructed `section` objects have indeterminate values. Add `= 0` (or `= -1` for `index`) to match the pattern used elsewhere.

### `src/Main.qml:141` — Dead `replaceView` signal connection

`NavigationState.replaceView.connect(...)` calls `stackView.replace(null, newView)`, but `replaceView` is only emitted by `setCurrentView()`, which is never called from QML. This is dead code that also violates the AGENTS.md rule: "Use `StackView.push()`/`pop()` — NOT `replace` — to preserve history." Remove the signal, slot, and connection.

### `src/modules/ContentDisplayModule/CMakeLists.txt:1` — Wrong file comment

Line 1 says `# HistoryModule CMakeLists.txt` — this is the ContentDisplayModule.

### `src/modules/HeaderModule/CMakeLists.txt:1` — Wrong file comment

Line 1 says `# HistoryModule CMakeLists.txt` — this is the HeaderModule.

## Low

### `src/wikipedia_page_client.cpp:27` — `stripHtml()` handles only 6 HTML entities

`&amp;`, `&lt;`, `&gt;`, `&quot;`, `&#39;`, `&nbsp;` are decoded. Numeric entities (`&#x27;`, `&#169;`), `&apos;`, and others are left as-is. For Wikipedia image descriptions this is usually sufficient, but a `QTextDocument`-based approach would be more complete.

### `src/wikipedia_page_client.cpp:271` — `parsePageWithImages()` stores image titles in the `imageUrls` field

The comment says "caller fetches URLs" — the caller (`onPageWithImagesReply`) extracts titles from `imageUrls`, clears it, then calls `fetchImageUrlsFromTitles()`. This overloading of `imageUrls` is fragile; a dedicated `imageTitles` field would be clearer (the field already exists on the `page` struct).

### `src/modules/HeaderModule/HeaderModel.cpp:51` — `renderSvgToImage()` reads the SVG file twice

`calculateSvgScaleFactor()` opens and reads the file (in `calculateSvgScaleFactor()`), then `renderSvgToImage()` opens and reads the same file again (in `renderSvgToImage()`). Pass the loaded `QByteArray` or `QSvgRenderer` between the two calls.

### `src/SearchScreen.qml:6` — Inconsistent versioned imports

Uses `import wikipedia_qt 1.0` while most other files use unversioned `import wikipedia_qt`. AGENTS.md notes versioned imports appear in "older files" — worth normalizing.

## Recent gallery coverage

The gallery navigation and image-details change (`e134cab`) has regression
coverage in `ImageSelectionStateTest` and `QmlTests.tst_images`: navigation stops
at either end, standalone selection clears gallery context, duplicate selection
does not repeat detail requests, failed/missing details fall back to the thumbnail
and caption, and stale details cannot overwrite a newer selection. QML tests
exercise arrow keys in embedded and fullscreen viewers, caption updates, and
shortcut isolation for covered viewers. Viewer preference coverage checks gallery
recreation within one process.

These gallery checks supplement the resolved gallery Back and invalid-navigation
findings; they do not resolve the remaining page-client or navigation findings.
Gallery tests were not rerun for the on-this-day fix; the focused home-client
test run is recorded with its resolved finding above.

## Summary

| Severity | Unresolved count |
|----------|-------|
| Critical | 0        |
| High     | 2        |
| Medium   | 6        |
| Low      | 4        |

The image gallery Back button, invalid `navigateToView()`, and ignored
on-this-day date arguments are resolved and covered by regression tests.
There are six unresolved medium-severity findings. The remaining `views.at()` crash risk in
`setCurrentView()` and the missing `<map>` include are quick, safe fixes. The dead
code in `getPageById()` wastes API bandwidth on every article load.
