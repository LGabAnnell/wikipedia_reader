Medium

src/modules/ImageDisplayModule/ImageGallery.qml:183 — A rapid double-click on a gallery cell pushes the in-app viewer twice navigateToView emits pushView, and StackView.push() does not deduplicate, so two clicks before the first push finishes leave two ImageView items on the stack — the first Back lands on a duplicate viewer. NavigationState.cpp already guards this for the history view (top-item check); apply the same top-item guard for Constants.imageView, or skip when stackView.busy is true.

src/modules/ImageDisplayModule/ImageGallery.qml:183 — The in-app viewer has no Escape handling, unlike the fullscreen window ImageFullscreenWindow.qml claims Escape before the underlying article window (lines 34-38), but when the switch is off, the pushed ImageView inherits nothing — Escape falls through to the article's Escape shortcut (hide Find, per the comment in tests/qml/tst_images.qml). Add an Escape shortcut in ImageView.qml that emits backRequested() so both display modes behave identically.

tests/qml/tst_images.qml:127 — Test functions share the persisted setting, coupling them to execution order init() does not reset the fullscreen setting, so test_selecting_gallery_image_shows_caption and test_selecting_image_with_fullscreen_disabled rely on checked === true holding only because earlier tests toggle it back on (or the config dir is fresh). Any test failing after toggling off cascades into confusing failures in the others. Reset the preference explicitly in init() (or cleanup()), the same way history and image selection are reset.


Low

docs/full_testing.md:135 — tst_images row does not cover the new behavior The table still describes only the fullscreen-window flow; the new settings-persistence test and the in-app viewer navigation (push on cell click, pop via backRequested, no main-window visibility change) are not mentioned. AGENTS.md section 7 names this file as the authoritative test guide, so it should be updated alongside the tests.

src/modules/ImageDisplayModule/ImageGallery.qml:16 — Qt.resolvedUrl() around an absolute local path is a no-op and slightly misleading StandardPaths.writableLocation(...) returns an absolute path, so resolvedUrl adds nothing on Linux and would mangle a Windows drive-letter path. A plain string (or QUrl.fromLocalFile) expresses the intent more directly.

Notes: I did not flag the QSettings-into-fresh-AppConfigLocation concern — the passing test_viewer_choice_survives_gallery_recreation shows writes succeed against a fresh temp config dir. Mouse-Back parity with the new in-app path is fine (NavigationState's event filter pops the main stack). This is a static review plus one test run; the other QML test files were not re-run.