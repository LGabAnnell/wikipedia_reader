# Wikipedia Reader

A Qt 6 / QML desktop application for searching and reading Wikipedia articles,
exploring article sections and images, and keeping browsing history.

## Build

Requires CMake, a C++20 compiler, Qt 6.8 or newer (Quick, Core, Network, Sql,
Gui, Svg, Widgets and SvgWidgets), tinyxml2, and Lexbor 3.0 or newer.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/bin/appwikipedia_qt
```

See [Flatpak packaging](packaging/README.md) for sandboxed builds and bundles,
and [testing](TESTING.md) for the test suite.

## License

Wikipedia Reader's application source is licensed under the GNU General Public
License, version 3 or (at your option) any later version
(`GPL-3.0-or-later`). See [LICENSE](LICENSE) for the full license text.

AppStream metadata is dedicated to the public domain under `CC0-1.0`.
Third-party dependencies and Wikipedia content retain their respective licenses.
This project is independent of the Wikimedia Foundation.
