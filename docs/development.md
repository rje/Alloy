# Development and observed evidence

Alloy currently builds a native **UI shell**. C++20 and Qt 6 Widgets/Network are
required. Open CASCADE is discovered optionally, but is not linked or integrated
into the app yet. No sketch solver is selected or integrated. There is no CAD
document, geometry, editable feature history, file persistence, import, or export.

## Build and run

On Apple silicon macOS with Homebrew:

```sh
brew install cmake ninja qtbase opencascade
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build build
./build/alloy.app/Contents/MacOS/alloy --ready-file /tmp/alloy-ready.json
```

On other platforms provide the Qt installation prefix through CMake as needed.
CMake produces `build/alloy` on Linux. Windows and Linux builds have not been
verified. `-DALLOY_DISCOVER_OCCT=OFF` deliberately builds the same shell without
kernel discovery. For nonstandard kernel installs set `OpenCASCADE_DIR` to the
folder containing `OpenCASCADEConfig.cmake`. Finding OCCT is not evidence of
geometry integration. Qt's built-in widget style happens to be named `reference CAD application`;
that open-source Qt style is unrelated to running the reference CAD application.

The public [Qt Widgets documentation](https://doc.qt.io/qt-6/qtwidgets-index.html)
and [QLocalServer documentation](https://doc.qt.io/qt-6/qlocalserver.html) describe
the UI/IPC foundations. The public
[OCCT visualization guide](https://www.occt3d.com/dev/doc/overview/html/occt_user_guides__visualization.html)
describes its visualization and interactive-object facilities for a future
viewport. These are permitted open-source foundation references, retrieved
2026-10-11. Kernel integration and distribution/license packaging remain
separate work.

## Source map

- `src/main.cpp`: native entrypoint, private endpoint setup, startup discovery.
- `src/AlloyWindow.{h,cpp}`: shell, shared command registry/state, reference view,
  command search, real-widget screenshot.
- `src/ControlServer.{h,cpp}`: newline JSON transport and structured dispatch.
- `tools/alloyctl.py`: stdlib CLI for discovery, execution, state, and capture.
- `tests/shell_api.py`: real app entrypoint/API smoke test.
- `docs/research/interface-reference.md`: public-only interface findings, source
  provenance, implementation guesses, and visual evidence limits.

The initial code intentionally keeps shell state small. The next geometry slice
should introduce authored document state and evaluated shapes separately; the
reference-grid renderer is not the future geometry model.

## Verification

```sh
ctest --test-dir build --output-on-failure
# Or run the same smoke test explicitly:
python3 tests/shell_api.py --app build/alloy.app/Contents/MacOS/alloy --output build/evidence
```

A desktop session is required for native-window evidence. Headless Qt plugins
may help CI later, but such captures must be labeled separately. Do not replace
the real-app test with an independent model of state. See
[agent-control.md](agent-control.md) for operation/capture commands.

Observed on **2026-10-11**, Apple silicon macOS, Apple Clang 17.0.0, Python 3.14.5,
Qt 6.11.2 (Homebrew `qtbase` 6.11.2_1), OCCT 7.9.3 (7.9.3_1):

| Evidence | Result and boundary |
| --- | --- |
| CMake/configure | Qt Widgets/Network found; OCCT 7.9.3 discovered |
| Native build | Debug app compiled and linked with warning flags |
| Real-app API | CTest `shell_api` passed; commands, state, errors, search lifecycle, captures, exit |
| Geometry/kernel | Not implemented or tested |
| Keyboard/mouse | Physical S, search Home/Return, and Cmd+Option+B checked; full physical input parity not verified |
| Visual capture | Main-window PNG inspected; no clipping or missing major shell regions at 1440×900 logical size |
| Product workflow | No sketch-to-solid or artist workflow acceptance yet |

Generated, ignored local evidence:

- `build/evidence/app.log`: app startup endpoint/log.
- `build/evidence/shell-api.json`: initial/final authoritative state, command
  transcript, capture dimensions and SHA-256.
- `build/evidence/alloy-shell.png`: restored shell, 2880×1800 physical pixels,
  device scale 2 at 1440×900 logical size.
- `build/evidence/alloy-shell-altered.png`: different actual UI state with hidden
  panels/grid/axes, for a basic check that automation affects rendering.

The resumed-session [shell baseline](validation/current-shell.md) also records
selected physical-input checks and preserves a public-safe Alloy capture.

The screenshots were visually inspected. The shell has readable layout and
truthful unavailable-modeling indicators. Toolbar symbols are simple geometric
text glyphs, not production icons. The Browser is fixed-width/full-height, the
orientation marker is a label rather than an interactive cube, and the reference
grid uses fixed orthographic bases with pan/zoom, without arbitrary orbit,
perspective, picking, snapping, or model rendering. Published reference images
are unmatched in size, scale, OS, and design state; no pixel-match claim is made.

Ordinary setup installed `qtbase` and `opencascade` with their Homebrew
dependencies; Homebrew also upgraded `hwloc` from 2.14.0 to 2.15.0. No proprietary
application was launched or inspected, and no package/service was published.
