# Testing

## Run the tests

Configure with `BUILD_TESTING=ON` before building:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build -j14
ctest --test-dir build --output-on-failure
```

Run the QML tests as a group or select one test file:

```bash
ctest --test-dir build -R QmlTests --output-on-failure
ctest --test-dir build -R '^QmlTests\.tst_article$' --output-on-failure
ctest --test-dir build -R '^QmlTests\.tst_images$' --output-on-failure
ctest --test-dir build -R '^QmlTests\.tst_history$' --output-on-failure
```

`BUILD_TESTING` defaults to `OFF`. The QML tests run with `QT_QPA_PLATFORM=offscreen`; each `tests/qml/tst_*.qml` file is registered as a separate CTest process.

## Current coverage

| Area | Tests in place |
| --- | --- |
| C++ backends | Qt Test coverage for state, HTML processing, history database, API clients, search display, and sidebar layout. |
| QML smoke | Singleton registration and initial state; Sidebar creation without requests or warnings (`tst_smoke.qml`). |
| Search | Blank input, mouse and Enter submission, pending controls, populated and empty responses, and recovery after an error (`tst_search.qml`). |
| Article | Select a search result, verify pending loading and rendered article content, and display an article load error without recording a failed visit (`tst_article.qml`). |
| Inline images | Thumbnail-to-original URL conversion and full-resolution image links in rendered article HTML (`tst_inline_images.qml`). |
| Gallery image | Load an article through the fake API, open its gallery, select an image, and verify the selected URL and description in `GlobalState` and the full-screen image and caption in `ImageView` (`tst_images.qml`). |
| History | Load two fixture articles through `GlobalState`, then revisit one; verify visit order and that the revisit moves it to the front without a duplicate (`tst_history.qml`). |

The gallery test uses the committed `tests/qml/fixtures/fixture.png`. The article, image, history, and search tests use the fake network transport; unexpected requests fail the test. QML tests check for warnings. The article test resets page, search, error, history, and navigation state between its success and failure cases.

## QML test harness

`tests/qml/qml_test_main.cpp` uses `QUICK_TEST_MAIN_WITH_SETUP` to register the real state singletons before importing the `wikipedia_qt` QML module. It installs the SVG provider and exposes `networkFixtures` and `testSupport` to the tests. The fake network manager matches requests to committed or test-defined fixtures, records requests, and can defer replies to test loading states. Both the C++ clients and QML image loading use this transport.

The harness sets `XDG_DATA_HOME` to a temporary directory before creating `HistoryState`. Each QML test file runs in its own process, so its SQLite history database is isolated from user data and other test files.

Add further QML coverage by creating a `tst_*.qml` file in `tests/qml/`. CMake discovers it automatically. Use fixture URLs for every request, stable `objectName` selectors for controls, and `tryCompare` or `tryVerify` for asynchronous behavior. Check `testSupport.qmlWarningCount`; unexpected network requests already fail immediately.

## Remaining QML coverage

- Back and Home navigation through the application stack.
- Section loading and scrolling to the selected heading.
- Home feed content and partial failure behavior.

The history QML test checks visits within one app run. Persistence across restarts is outside this QML test; database behavior has separate C++ tests.
