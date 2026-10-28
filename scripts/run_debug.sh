#!/usr/bin/env bash

set -euo pipefail

# Build and run appwikipedia_qt in debug mode.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build/debug"

if [[ ${1:-} == "rebuild" ]]; then
    echo "Rebuilding the debug project..."
    rm -rf "$BUILD_DIR"
elif [[ $# -gt 0 ]]; then
    echo "Usage: $0 [rebuild]" >&2
    exit 2
fi

"$SCRIPT_DIR/build_debug.sh"

echo "Running appwikipedia_qt in debug mode..."
QT_LOGGING_RULES="default.debug=true" "$BUILD_DIR/bin/appwikipedia_qt"
