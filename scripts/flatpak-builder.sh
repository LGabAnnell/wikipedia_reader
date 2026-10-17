#!/usr/bin/env bash
set -euo pipefail

if command -v flatpak-builder >/dev/null 2>&1; then
    exec flatpak-builder "$@"
fi

# The Builder Flatpak sets private XDG directories. Do not forward those to
# host-side SDK discovery: user-installed runtimes live in the host directories.
exec flatpak run --command=sh org.flatpak.Builder -c '
    unset XDG_DATA_HOME XDG_CONFIG_HOME XDG_CACHE_HOME
    exec flatpak-builder "$@"
' flatpak-builder "$@"
