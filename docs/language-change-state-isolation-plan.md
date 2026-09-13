# Language Change State Isolation Plan

Changing the selected Wikipedia language must be treated as a complete data-domain transition, not merely as a base-URL update. Wikipedia page IDs are local to each language edition, so state or network responses from one language must never be consumed in another.

## 1. Reset language-bound application state

Update `GlobalState::setLanguage()` to clear all transient state associated with the previous language:

- Search results
- Current page
- Current-page sections
- Current image URL and description
- Current section index
- Loading flags and obsolete errors
- Article cache

Emit the corresponding change signals so existing QML views immediately become empty.

Use value initialization such as `page{}` when resetting state. The integer fields in the data models should also be initialized to zero to prevent an empty page from containing an indeterminate page ID.

`SearchBarModel` and `SectionModel` maintain local loading or result state in addition to `GlobalState`. Their `languageChanged` handlers should reset that local state as well.

## 2. Make browsing history language-aware

Language must be part of an article's identity:

```text
(language, page_id)
```

Add a `language` column to the history table, a unique index on `(language, page_id)`, and an index on `(language, timestamp)`.

History database operations should become language-scoped:

```cpp
loadHistory(language, limit)
addToHistory(language, title, pageId, timestamp)
clearHistory(language)
removeOldestItems(language, limit)
```

`HistoryState` should maintain an active language and reload its visible history whenever that language changes. `GlobalState::setCurrentPage()` should explicitly pass the current language when adding an article to history.

Existing database rows cannot reliably be assigned a language because the old schema did not record it. The safest migration is to discard or hide legacy rows rather than assume they are English; otherwise a page ID originally obtained from another edition could still be interpreted against English Wikipedia.

The existing **Clear History** action should clear history for the active language. A separate clear-all operation can be added later if required.

There is also an adjacent trimming issue: the database trimming condition is checked after the in-memory collection has already been reduced to the maximum size, so it is never reached. Run language-scoped database trimming after each successful insertion.

## 3. Invalidate in-flight requests

Add a monotonically increasing generation and active-reply tracking to every Wikipedia client:

```cpp
QString m_language;
quint64 m_generation = 0;
QSet<QNetworkReply *> m_activeReplies;
```

When the language actually changes:

1. Increment the generation.
2. Update the language and base URL.
3. Abort tracked replies.
4. Ignore aborted or stale callbacks without emitting user-facing errors.

Every request should capture its generation:

```cpp
const quint64 generation = m_generation;

connect(reply, &QNetworkReply::finished, this,
        [this, reply, generation] {
            if (generation != m_generation) {
                reply->deleteLater();
                return;
            }

            // Parse and emit the response.
        });
```

Generation validation is the correctness mechanism; aborting requests is an optimization. Both are useful because a reply can finish while cancellation is occurring.

Apply this consistently to:

- `WikipediaPageClient`
- `WikipediaSearchClient`
- `WikipediaHomeClient`
- `WikipediaFeaturedClient`

The captured request context must also be propagated through chained operations:

- Title resolution to page-by-ID loading
- Initial page request to HTML request to math-image requests
- Featured article to page-image loading
- Random title to article-summary loading
- Image-title lookup

A stale callback must return before starting the next request. Chained request URLs should be built from the captured language or base URL rather than the mutable current language.

The blocking `WikipediaPageClient::fetchImageUrlsFromTitles()` implementation should ideally be converted to an asynchronous chain as part of this work, because its nested event loop complicates cancellation and generation handling.

## 4. Clear and refresh home content

When `HomeModel` receives `GlobalState::languageChanged`:

1. Change the language of all three clients, invalidating their old requests.
2. Clear the featured article fields, news items, on-this-day events, and did-you-know items.
3. Emit `featuredArticleUpdated`, `featuredArticleImageUpdated`, and `dataUpdated`.
4. Call `fetchHomeData()`.

This makes old cards disappear immediately and automatically fetches content for the newly selected language.

## 5. Recommended transition sequence

`GlobalState::setLanguage()` should perform the transition in this order:

```text
Validate the new language
    |
Update and invalidate GlobalState clients
    |
Clear transient GlobalState data
    |
Switch HistoryState's active language
    |
Emit languageChanged
    |
Feature models invalidate their clients, clear local data, and refetch
```

## 6. Tests

Add coverage for the following behavior:

- Changing language clears every transient `GlobalState` property.
- Identical page IDs in English and French coexist independently in history.
- History loading, deduplication, trimming, and clearing are language-scoped.
- An English reply completed after switching to French emits no result or error.
- Stale title resolution cannot initiate a French request with an English page ID.
- Stale random-title and featured-article chains cannot repopulate `HomeModel`.
- Rapid `en -> fr -> de` changes leave only German results visible.

