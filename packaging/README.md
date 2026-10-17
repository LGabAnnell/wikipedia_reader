# Flatpak packaging

Application ID: `io.github.lgabannell.wikipedia_reader`.
Executable: `appwikipedia_qt`.

The root manifest builds the working checkout, including uncommitted source
changes, with KDE's Qt 6.11 runtime and SDK. tinyxml2 11.0.0 and Lexbor 3.0.0
are built from source archives with pinned SHA-256 checksums. Dependency license
files, including Lexbor's NOTICE, are included in the package.

## Set up

Install Flatpak using your distribution's package manager, then run:

```sh
flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.flatpak.Builder org.kde.Sdk//6.11 org.kde.Platform//6.11
```

## Build

From the repository root:

```sh
./scripts/flatpak-builder.sh \
  --force-clean --user --install-deps-from=flathub --jobs=8 \
  --repo=flatpak-repo flatpak-build io.github.lgabannell.wikipedia_reader.yml
```

`--force-clean` replaces the disposable `flatpak-build` directory on each run.
Build output, the repository and builder caches are ignored by Git and excluded
from the source copied into the sandbox.

The helper uses a native `flatpak-builder` when installed, otherwise the Builder
Flatpak. In the latter case it clears that app's private XDG paths before
host-side SDK discovery, so user-installed runtimes can be found.

CMake's `WIKIPEDIA_FLATPAK=ON` option installs this project's QML modules,
plugins and backing libraries. The sandbox uses Qt, its QML modules, platform
plugins and SQLite driver from the KDE runtime. Other Linux installations keep
the existing full Qt deployment behavior.

## Run without installing

```sh
./scripts/flatpak-builder.sh \
  --run flatpak-build io.github.lgabannell.wikipedia_reader.yml appwikipedia_qt
```

The package grants network access, Wayland, fallback X11, shared IPC and GPU
rendering. `QML_IMPORT_PATH=/app/qml` exposes the app's own modules alongside
the runtime's modules. No additional home or host filesystem access is needed.
History is saved through `QStandardPaths` in the sandbox's private data storage.

## Export and install a bundle

```sh
flatpak build-bundle flatpak-repo wikipedia-reader.flatpak \
  io.github.lgabannell.wikipedia_reader \
  --runtime-repo=https://dl.flathub.org/repo/flathub.flatpakrepo

flatpak install --user wikipedia-reader.flatpak
flatpak run io.github.lgabannell.wikipedia_reader
```

The bundle contains the application and bundled dependencies. Flatpak downloads
the KDE runtime separately if needed. Distribute the matching corresponding
source with your GPL release; the local-directory manifest alone does not
identify the exact source used to build a bundle.

## Validate

```sh
desktop-file-validate packaging/io.github.lgabannell.wikipedia_reader.desktop
appstreamcli validate --no-net packaging/io.github.lgabannell.wikipedia_reader.metainfo.xml
```

Exercise search, article navigation, sections, image galleries and clipboard.
Restart the installed app to verify history persistence. Test both Wayland and
X11 sessions when available. Check that the package works without access to the
host's Qt installation or this checkout.

## Before a Flathub submission

This manifest is AI-assisted and intended for local builds and direct
distribution. Flathub's [current AI policy](https://docs.flathub.org/docs/for-app-authors/requirements#generative-ai-policy)
prohibits AI-generated or AI-assisted manifests and AI-agent submission PR
interactions. Do not submit this manifest to Flathub. A maintainer must author
the submission packaging independently and disclose other included AI-generated
material as required by that policy.

Publication also needs maintainer-supplied release details:

- Tag a stable release and pin its public HTTPS Git commit or release archive
  in the submission manifest instead of the local directory source.
- Add real screenshots with reachable URLs, a release version and date, and an
  accurate content rating to the AppStream metadata.
- Review the public app name and icon for compliance with Flathub's naming
  and trademark requirements.
- Recheck the latest supported KDE runtime, run the Flathub linter on both the
  manifest and built repository, and test the supported architectures.
- Follow the [submission guide](https://docs.flathub.org/docs/for-app-authors/submission)
  to open a human-authored PR against `flathub/flathub`'s `new-pr` branch.

The current metadata omits screenshots, release dates and a content rating
rather than inventing them. Passing local metadata validation does not mean a
package is ready for Flathub publication.
