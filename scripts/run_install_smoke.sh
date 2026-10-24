#!/usr/bin/env bash

# Install, relocate, and launch the deployed application without using build
# tree library or QML paths. The optional second argument keeps diagnostics in
# a caller-owned directory for CI artifact upload.
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${1:-$PROJECT_DIR/build}"
LOG_DIR="${2:-}"

if [[ ! -f "$BUILD_DIR/cmake_install.cmake" ]]; then
    echo "No configured build found at $BUILD_DIR" >&2
    exit 2
fi
BUILD_DIR="$(cd -- "$BUILD_DIR" && pwd)"

if ! grep -Eq '^WIKIPEDIA_BUNDLE_QT:BOOL=ON$' "$BUILD_DIR/CMakeCache.txt"; then
    echo "Install smoke requires a Qt-bundled build. Configure with -DWIKIPEDIA_BUNDLE_QT=ON (and install patchelf for Linux packaging)." >&2
    exit 2
fi

TEMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/wikipedia-reader-install-smoke.XXXXXX")"
if [[ -z "$LOG_DIR" ]]; then
    LOG_DIR="$TEMP_DIR/logs"
fi
mkdir -p "$LOG_DIR"
INSTALL_PREFIX="$TEMP_DIR/install"
RELOCATED_PREFIX="$TEMP_DIR/relocated"

cleanup() {
    status=$?
    if (( status == 0 )); then
        rm -rf -- "$TEMP_DIR"
    else
        echo "Install smoke failed; temporary install and diagnostics remain at $TEMP_DIR" >&2
    fi
    exit "$status"
}
trap cleanup EXIT

cmake --install "$BUILD_DIR" --prefix "$INSTALL_PREFIX" 2>&1 | tee "$LOG_DIR/install.log"
mv -- "$INSTALL_PREFIX" "$RELOCATED_PREFIX"

APP="$RELOCATED_PREFIX/bin/appwikipedia_qt"
if [[ ! -x "$APP" ]]; then
    echo "Installed executable is missing: $APP" | tee -a "$LOG_DIR/smoke.log" >&2
    exit 1
fi

QT6_DIR="$(sed -n 's/^Qt6_DIR:[^=]*=//p' "$BUILD_DIR/CMakeCache.txt" | head -n 1)"
if [[ -z "$QT6_DIR" || ! -d "$QT6_DIR" ]]; then
    echo "Cannot determine the selected Qt package directory from Qt6_DIR=$QT6_DIR" \
        | tee -a "$LOG_DIR/smoke.log" >&2
    exit 1
fi
QT_PREFIX="$(cd -- "$QT6_DIR/../../.." && pwd -P)"
if find "$RELOCATED_PREFIX" -name 'libQt6Core.so*' -print -quit | grep -q .; then
    BUNDLED_QT=1
else
    BUNDLED_QT=0
fi

env -u LD_LIBRARY_PATH ldd "$APP" | tee "$LOG_DIR/ldd.log"
if grep -q 'not found' "$LOG_DIR/ldd.log"; then
    echo "Relocated executable has unresolved shared libraries" | tee -a "$LOG_DIR/smoke.log" >&2
    exit 1
fi
if grep -Fq "$BUILD_DIR" "$LOG_DIR/ldd.log"; then
    echo "Relocated executable still resolves a library from the build tree" \
        | tee -a "$LOG_DIR/smoke.log" >&2
    exit 1
fi
if (( BUNDLED_QT )) && grep -E "libQt6[^ ]* => ${QT_PREFIX}/" "$LOG_DIR/ldd.log"; then
    echo "Relocated executable still resolves bundled Qt from the SDK at $QT_PREFIX" \
        | tee -a "$LOG_DIR/smoke.log" >&2
    exit 1
fi

mkdir -p "$TEMP_DIR/home" "$TEMP_DIR/config" "$TEMP_DIR/data" "$TEMP_DIR/cache"
set +e
timeout 30s env \
    -u LD_LIBRARY_PATH \
    -u QML_IMPORT_PATH \
    -u QML2_IMPORT_PATH \
    -u QT_PLUGIN_PATH \
    -u QT_QPA_PLATFORM_PLUGIN_PATH \
    QT_QPA_PLATFORM=offscreen \
    QT_DEBUG_PLUGINS=1 \
    HOME="$TEMP_DIR/home" \
    XDG_CONFIG_HOME="$TEMP_DIR/config" \
    XDG_DATA_HOME="$TEMP_DIR/data" \
    XDG_CACHE_HOME="$TEMP_DIR/cache" \
    "$APP" --smoke-test 2>&1 | tee "$LOG_DIR/smoke.log"
status=${PIPESTATUS[0]}
set -e

if (( status != 0 )); then
    echo "Relocated application smoke test exited with status $status" >&2
    exit "$status"
fi
if ! grep -q 'Wikipedia Reader startup smoke test passed' "$LOG_DIR/smoke.log"; then
    echo "Application exited without confirming root QML startup" >&2
    exit 1
fi

echo "Relocated install smoke test passed" | tee "$LOG_DIR/result.log"
