# Qt Quick Test Initial Setup Specification

## Goal

Add a deterministic Qt Quick Test harness that can load the application's real QML modules without accessing Wikipedia or the user's persistent data. This work establishes testing infrastructure only; feature-level test coverage is outside its scope.

## Build integration

- Add the Qt `QuickTest` component when `BUILD_TESTING=ON`.
- Add one test executable, `QmlTests`, under `tests/qml/` and link it to `Qt::QuickTest`, `Qt::Qml`, `Qt::Quick`, and the application QML modules it loads.
- Implement the runner with `QUICK_TEST_MAIN_WITH_SETUP`.
- Register it with CTest and run it with `QT_QPA_PLATFORM=offscreen` and the build directory on the QML import path.
- Keep live-network tests separate and opt-in through the existing `RUN_NETWORK_TESTS` convention.

Proposed layout:

```text
tests/qml/
├── CMakeLists.txt
├── qml_test_main.cpp
├── support/
│   ├── FakeNetworkAccessManager.*
│   ├── FakeNetworkReply.*
│   └── NetworkFixtureController.*
├── fixtures/
└── tst_smoke.qml
```

## Runner setup

Before each QML test file is loaded, the runner must:

1. Add the application's generated QML-module directory to the engine import path.
2. Register `GlobalState`, `HistoryState`, and `NavigationState` using the same QML names as the application.
3. Install required image providers and test support objects.
4. Redirect application data to a unique temporary directory.
5. Install the fake network transport before constructing any state object or QML model.

Each QML test file must run in its own process. This avoids sharing singleton registrations, SQLite connections, state caches, or pending replies between the separate QML engines created by Qt Quick Test.

## Network isolation

All Wikipedia clients must obtain their `QNetworkAccessManager` from an injectable factory. Production uses a normal manager; the test runner supplies `FakeNetworkAccessManager`.

The fake transport must:

- Match requests by HTTP method, host, path, and query parameters.
- Return committed JSON, HTML, or image fixtures asynchronously.
- Support successful replies, HTTP errors, and network errors.
- Record received requests for assertions.
- Fail the test on an unexpected request.
- Never fall back to the real network.

The QML engine must use the same fake transport for URLs loaded directly by QML, such as images.

## Test isolation

- History must use a database inside the temporary application-data directory.
- Test fixtures must not depend on the current date, locale, network, or existing user history.
- The runner must destroy state objects and close the test database before removing the temporary directory.
- QML tests should use `tryCompare`, `tryVerify`, or `SignalSpy` for asynchronous behavior; fixed sleeps are not allowed.

## Initial smoke test

Add one infrastructure smoke test only. It must load an application component through its normal module import, verify that it is created without QML warnings, and confirm that no unexpected network request occurs. Its purpose is to prove that imports, type registration, headless rendering, isolation, and CTest integration work.

## Acceptance criteria

The setup is complete when:

- `cmake -B build -DBUILD_TESTING=ON` configures with Qt Quick Test enabled.
- `cmake --build build` builds the QML test runner.
- `ctest --test-dir build` runs the smoke test headlessly.
- The test passes without internet access and without reading or modifying the user's history database.
- Any unregistered network request causes an immediate, descriptive failure.
- Existing C++ tests and the application build remain operational.

## Out of scope

Search, article, navigation, home-screen, section, gallery, and history behavior tests are deferred. Refactoring unrelated production behavior and adding CI configuration are also excluded from this initial setup.
