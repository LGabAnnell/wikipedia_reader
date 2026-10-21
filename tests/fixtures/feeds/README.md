# Recorded feed samples

Captured on 2026-10-21 from:

- https://api.wikimedia.org/feed/v1/wikipedia/en/featured/2026/10/21
- https://api.wikimedia.org/feed/v1/wikipedia/fr/featured/2026/10/21

These are reduced real responses, not invented example articles. Only the fields
consumed by the featured/news clients are retained; mostread is limited to two
articles and news to one story. Article field values and linked articles are
unchanged. English includes `tfa`; this French response does not.

The samples verify parsing against observed data. They cannot verify the live
endpoint: run `WikipediaFeedLiveTest` when changing feed hosts, paths, or schemas.
Do not move fixture URLs to a new endpoint without checking its real response.
