# Development and observed evidence

Alloy currently builds a native **UI shell with a hardware-accelerated OCCT 3D
viewport**. C++20, Qt 6.11.2 Widgets/Network, and OCCT 7.9.3 are required and the
CMake package versions are pinned to this verified baseline. OCCT visualization
is linked and integrated; its reference grid and axes are presentation objects,
not authored CAD geometry. No sketch solver is integrated. There is no CAD
document, editable feature history, file persistence, import, or export.

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
verified. OCCT is now mandatory; there is no QPainter fallback. For nonstandard
installs set `OpenCASCADE_DIR` to the folder containing `OpenCASCADEConfig.cmake`.
Visualization integration is not evidence of authored geometry integration. Qt's built-in widget style happens to be named `reference CAD application`;
that open-source Qt style is unrelated to running the reference CAD application.

The public [Qt Widgets documentation](https://doc.qt.io/qt-6/qtwidgets-index.html)
and [QLocalServer documentation](https://doc.qt.io/qt-6/qlocalserver.html) describe
the UI/IPC foundations. The public
[OCCT visualization guide](https://www.occt3d.com/dev/doc/overview/html/occt_user_guides__visualization.html)
describes the integrated visualization and interactive-object facilities. These are permitted open-source foundation references, retrieved
2026-10-11. Authored geometry and distribution/license packaging remain separate work.
Native macOS integration was checked against the installed 7.9.3 public headers
and public version-pinned source for
[Cocoa_Window](https://github.com/Open-Cascade-SAS/OCCT/blob/V7_9_3/src/Cocoa/Cocoa_Window.mm)
and [OpenGl_Window](https://github.com/Open-Cascade-SAS/OCCT/blob/V7_9_3/src/OpenGl/OpenGl_Window_1.mm),
retrieved 2026-10-11. OCCT performs native backing-pixel conversion; Qt provides
logical widget dimensions and device pixel ratio.

## Source map

- `src/main.cpp`: native entrypoint, private endpoint setup, startup discovery.
- `src/AlloyWindow.{h,cpp}`: shell, shared commands/state, search, screenshot composition.
- `src/ReferenceView.{h,cpp}`: native OCCT AIS/V3d/OpenGL view, grid/axes, camera,
  live driver diagnostics, Retina framebuffer capture; no document objects.
- `src/ControlServer.{h,cpp}`: newline JSON transport and structured dispatch.
- `tools/alloyctl.py`: stdlib CLI for discovery, execution, state, and capture.
- `tests/shell_api.py`: real app entrypoint/API smoke test.
- `docs/research/interface-reference.md`: public-only interface findings, source
  provenance, implementation guesses, and visual evidence limits.

The initial code intentionally keeps shell state small. The next geometry slice
should introduce authored document state and evaluated shapes separately; the
OCCT scene is not the future geometry model.

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
| CMake/configure | Qt 6.11.2 and OCCT 7.9.3 pinned, required and linked |
| Native build | Debug app compiled and linked with warning flags |
| Real-app API | CTest `shell_api` passed 1/1; commands, state, errors, search lifecycle, native resize, captures, exit |
| GPU renderer | Live OCCT diagnostics: vendor Apple, device Apple M4, OpenGL `4.1 Metal - 90.5`, GLSL 4.10; hardware driver identified |
| Retina/framebuffer | DPR 2; native 1440×900 and 1200×800 windows captured at 2880×1800 and 2400×1600; native GL viewport dimensions checked before offscreen captures; isometric/top framebuffer hashes differ |
| Authored geometry/kernel | Not implemented or tested; only OCCT presentation grid/axes |
| Keyboard/mouse | GPU build physical Top and S/search Home/Return checked; prior QPainter-shell Cmd+Option+B remains historical evidence |
| Visual capture | Main-window PNG inspected; no clipping or missing major shell regions at 1440×900 logical size |
| Product workflow | No sketch-to-solid or artist workflow acceptance yet |

Generated, ignored local evidence:

- `build/evidence/app.log`: app startup endpoint/log.
- `build/evidence/shell-api.json`: initial/final authoritative state, command
  transcript, capture dimensions and SHA-256.
- `build/evidence/alloy-shell.png`: restored shell, 2880×1800 physical pixels,
  device scale 2 at 1440×900 logical size.
- `build/evidence/alloy-gpu-isometric.png`, `alloy-gpu-top.png`: camera-only
  change checked using the OCCT framebuffer SHA-256, separately from shell pixels.
- `build/evidence/alloy-gpu-resized.png`: 1200×800 logical native window, top
  view; 2400×1600 PNG on the verified Retina display.
- `build/evidence/alloy-shell-altered.png`: different actual UI state with hidden
  panels/grid/axes, for a basic check that automation affects rendering.

The resumed-session [shell baseline](validation/current-shell.md) also records
selected physical-input checks and preserves a public-safe Alloy capture. The
[GPU foundation receipt](validation/gpu-foundation.md) records the new hardware
viewport and separate native-compositor/keyboard observations.

The screenshots were visually inspected. The shell has readable layout and
truthful unavailable-modeling indicators. Toolbar symbols are simple geometric
text glyphs, not production icons. The Browser is fixed-width/full-height, the
viewport has OCCT reference axes rather than an interactive view cube. Named
orthographic cameras, pan and zoom are implemented; arbitrary orbit, perspective
controls, model picking, snapping, and authored model rendering remain pending. Published reference images
are unmatched in size, scale, OS, and design state; no pixel-match claim is made.

Ordinary setup installed `qtbase` and `opencascade` with their Homebrew
dependencies; Homebrew also upgraded `hwloc` from 2.14.0 to 2.15.0. No proprietary
application was launched or inspected, and no package/service was published.

## GPU qualification boundary

`app.state.viewport.gpu` reports live OCCT diagnostics, vendor, renderer, version,
initialization/error, native window units, requested framebuffer pixels, and
`hardwareAccelerated`. Known software renderers (llvmpipe, softpipe, swrast,
SwiftShader, or an identity containing software) are rejected for qualification;
unrecognized identities remain unverified. The smoke test fails unless a known
hardware driver is identified. This is driver-identity evidence, not a performance
benchmark or a guarantee of every future CAD workload. Failure/unverified status
is visible in the status bar. OpenGL initialization errors have no raster fallback.

`capture.screenshot` combines actual Qt chrome with OCCT `ToPixMap` GPU rendering
at the current viewport size and device scale. It is an offscreen framebuffer
render of the same scene/camera, not an OS-compositor screenshot; physical native
window observation remains a separate check. The smoke test does not yet qualify
picking, arbitrary orbit, cross-display scaling changes, long-running graphics
resource behavior, or Linux/Windows. No reference CAD application runtime was used for comparison.

This increment partially advances DOC-005 (GPU viewport) but does not close it or
GATE-010: those additionally require actual document geometry, selection/picking,
and the first useful-object cumulative workflow. GATE-000 records the earlier
shell baseline and remains separate.
