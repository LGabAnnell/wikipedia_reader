# Proposed fix for P1: give the stateful core one shared-library owner

Status: proposal only; the implementation has not been changed or validated.

## Problem

`CMakeLists.txt` declares `wikipedia_qt_core` as `STATIC`. Although its
sources are compiled once, the resulting object files can be copied into each
shared QML module that links the archive, as well as the application and test
executables. SearchBarModule and SidebarModule both link this target.

The archive includes `src/state/GlobalState.cpp`, which defines the nontrivial
static object `GlobalState::m_instance` (`QPointer<GlobalState>`). The review
reports that Linux symbol interposition resolves multiple copies to the same
storage, while their separate destructor registrations remain. At shutdown,
SearchBarModule releases the pointer's control block and SidebarModule later
accesses it. Passing functional tests does not detect this shutdown
use-after-free.

## Recommended change

Make `wikipedia_qt_core` explicitly shared so the backend implementation,
meta-objects, and singleton storage have one owner per process:

```diff
 # Shared backend implementation and Qt meta-objects, used by the app and tests.
-add_library(wikipedia_qt_core STATIC
+add_library(wikipedia_qt_core SHARED
```

Keep the current source list, `AUTOMOC`, public include directories, link
dependencies, and `qt_extract_metatypes(wikipedia_qt_core)`. Keep the existing
target links from the application, QML modules, and tests. They will reference
the same shared library rather than embed its backend objects. Explicit
`SHARED` makes this guarantee independent of `BUILD_SHARED_LIBS`.

Do not reintroduce backend source files or their generated moc objects into
consumer targets. The existing `wikipedia_qt` foreign-type registration should
continue to reference the core's meta-objects. `wikipedia_runtime` can retain
its current static definition: it contains navigation and is currently linked
only by the app and QML test executable, not by the shared feature modules.

Preserve the current singleton ownership and cleanup. Replacing `QPointer`
with a raw pointer, leaking its storage, or changing symbol binding would not
establish a single backend owner.

## Shared-library deployment and portability

Include the core in the existing installation rule:

```diff
-install(TARGETS appwikipedia_qt
+install(TARGETS appwikipedia_qt wikipedia_qt_core
     BUNDLE DESTINATION .
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
 )
```

Verify runtime discovery in both the build tree and the deployed application.
Installing the library alone does not guarantee that the executable or QML
modules can locate it. Use the deployment mechanism for each supported
platform; for a conventional Linux installation, ensure the loader searches
the installed library directory through an appropriate install RPATH or
system configuration. Include the core in macOS bundle deployment and place
its DLL with the executable on Windows.

The minimal change targets the reported Linux configuration with default
symbol visibility. If supporting Windows or hidden-visibility builds, add a
generated export header using CMake's `GenerateExportHeader`, expose its
directory to consumers, and annotate the public backend classes and gadget
types. Export/import handling must cover their meta-objects and static data,
including `GlobalState::m_instance`, which the inline `instance()` accessor
references. Validate those configurations before claiming portability.

## Validation and acceptance criteria

Use a fresh build directory to avoid stale archive or module artifacts:

```bash
cmake -S . -B build-p1 -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build-p1 -j14
mkdir -p build-p1/test-data
XDG_DATA_HOME="$PWD/build-p1/test-data" QT_QPA_PLATFORM=offscreen \
    ctest --test-dir build-p1 --output-on-failure
```

The offscreen environment accommodates the separate P2 issue during this
check; it does not resolve P2.

1. Confirm with `nm -C --defined-only` that `GlobalState::m_instance` is
   defined in `libwikipedia_qt_core.so` and has no definition in the executable
   or feature libraries. Use `readelf -d` or `ldd` to verify that the modules
   needing the backend resolve the same core library.
2. Run the smoke test under Valgrind through normal process exit. Obtain its
   exact executable path and arguments with:

   ```bash
   ctest --test-dir build-p1 -N -V -R '^QmlTests\.tst_smoke$'
   ```

   Run that command with `QT_QPA_PLATFORM=offscreen`, prefixing the executable
   with `valgrind --tool=memcheck --error-exitcode=99 --track-origins=yes`.
   Retain the test's working directory and environment. The QML harness
   already redirects application data into its own temporary directory.
   Require a passing smoke test and no invalid reads, invalid frees, or
   duplicate-destruction errors involving `GlobalState::m_instance` at exit.
   Compare unrelated Qt/runtime reports with the review baseline.
3. Require the full existing CTest suite to pass, including the QML search
   tests that exercise the shared state and injected network factory. Review
   any new QML registration or meta-object warnings.
4. Launch the deployed application from a temporary install or packaged
   location to check core-library discovery without relying on build-tree
   paths.

This proposal addresses P1. Preserving GUI-less backend test startup remains
the separate P2 fix.
