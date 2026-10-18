# Testing

The authoritative [testing guide](docs/full_testing.md) covers dependencies,
build commands, coverage, fixtures, and adding tests.

For an incremental local run:

```bash
./scripts/run_tests.sh
./scripts/run_tests.sh -R QmlTests
```

Configure dependency prefixes first if needed. The legacy `test.sh` deletes
`build/` on every invocation; prefer the incremental runner.
