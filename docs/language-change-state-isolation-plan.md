# Language Change State Isolation

Changing Wikipedia language is a transition between separate data domains. Page IDs, search results, article content, and home cards from one edition must not be used by another.

## Transition behavior

`SettingsState` remains the language authority. On a real language change it updates the selected language, emits `languageTransitionStarted()` so consumers synchronously invalidate work and clear edition-bound state, then emits `languageChanged()` so clients adopt the edition and data models refresh.

The current screen and navigation stack remain in place. Shared article content and cache are cleared; covered `ContentDisplay` snapshots are cleared too, so Back cannot reveal old-edition content. Selecting the same language does nothing. Home refreshes, and the latest nonblank search text is submitted once in the new edition.

Browsing history remains unchanged in this work. Its page-ID identity must be made language-aware in a separate database migration before history entries can safely cross language switches.

## Request isolation

All four Wikipedia clients use an internal request context containing the captured language and a monotonically increasing generation. They track active replies and expose `cancelPendingRequests()`. Cancellation advances the generation before aborting replies; callbacks check the captured context before emitting results or errors, updating state, or starting a chained request.

The context follows the full request chain, including title resolution, article HTML and math images, image metadata, featured article images, and random-title summaries. URLs for chained requests use the captured edition. Image metadata lookup is asynchronous so language changes can invalidate it without a nested event loop.

At each new article selection, the prior page request generation is invalidated before consulting the cache. A title resolution continues its original selection. Sections and gallery requests are canceled when their target changes or is cleared. Home’s independent feeds run in one refresh generation.

## Validation

The fake transport can defer a reply, optionally allow it to complete after abort, and complete a selected reply by URL. Tests verify that obsolete replies cannot publish data or start follow-up requests, new selections supersede old ones, searches restart once in the selected language, and retained article views clear without changing navigation depth. The complete CTest suite should run with `QT_QPA_PLATFORM=offscreen` for GUI tests.
