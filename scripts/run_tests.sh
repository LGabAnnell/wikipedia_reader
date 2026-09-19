#!/bin/bash

# Script to build and run the test suite.
#
# Usage:
#   ./scripts/run_tests.sh              # configure (if needed) + incremental build + ctest
#   ./scripts/run_tests.sh -r           # wipe build/ first, then configure + build + ctest
#   ./scripts/run_tests.sh -R <regex>   # only run tests matching the ctest regex
#   ./scripts/run_tests.sh -n           # also enable live-network integration tests
#
# See TESTING.md for the full test documentation.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

REBUILD=0
TEST_FILTER=""
NETWORK=0

usage() {
    echo "Usage: $0 [-r|--rebuild] [-R <regex>] [-n|--network] [-h|--help]"
    echo
    echo "Options:"
    echo "  -r, --rebuild      Wipe build/ and reconfigure from scratch before testing"
    echo "  -R <regex>         Only run tests whose name matches the ctest regex"
    echo "  -n, --network      Export RUN_NETWORK_TESTS=1 to enable live-network tests"
    echo "  -h, --help         Show this help"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -r|--rebuild)
            REBUILD=1
            shift
            ;;
        -R)
            if [[ $# -lt 2 ]]; then
                echo "Error: -R requires a regex argument" >&2
                exit 1
            fi
            TEST_FILTER="$2"
            shift 2
            ;;
        -n|--network)
            NETWORK=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Error: unknown option: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

if [[ $REBUILD -eq 1 ]]; then
    echo "Wiping build directory..."
    rm -rf "$BUILD_DIR"
fi

# Configure only when there is no existing cache or the cache was not
# configured with tests enabled.
NEED_CONFIGURE=0
if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
    NEED_CONFIGURE=1
elif ! grep -q "^BUILD_TESTING:BOOL=ON$" "$BUILD_DIR/CMakeCache.txt"; then
    NEED_CONFIGURE=1
fi

if [[ $NEED_CONFIGURE -eq 1 ]]; then
    echo "Configuring build with tests enabled..."
    cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
fi

echo "Building..."
cmake --build "$BUILD_DIR" -j14

echo "Running tests..."
if [[ $NETWORK -eq 1 ]]; then
    export RUN_NETWORK_TESTS=1
fi

CTEST_ARGS=(--test-dir "$BUILD_DIR" --output-on-failure)
if [[ -n "$TEST_FILTER" ]]; then
    CTEST_ARGS+=(-R "$TEST_FILTER")
fi

QT_QPA_PLATFORM=offscreen ctest "${CTEST_ARGS[@]}"
