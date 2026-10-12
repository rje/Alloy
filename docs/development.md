# Development and observed evidence

Alloy now creates and edits real OCCT boxes and cylinders in a hardware-accelerated
native viewport. Shared UI/API commands provide disposable previews, body picking,
geometry inspection, undo/redo and atomic native save/open. C++20, Qt 6.11.2
Widgets/Network and OCCT 7.9.3 are required; CMake pins those package versions.
Sketches, dependent modeling, import and export remain ahead.

## Build and run

On Apple silicon macOS with Homebrew:

```sh
brew install cmake ninja qtbase opencascade
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build build
./build/alloy.app/Contents/MacOS/alloy --ready-file /tmp/alloy-ready.json
```

Use File → Open for `examples/gauge-block.alloy`. Click a Timeline feature to
edit its dimensions, or Box/Cylinder to create another body. See
[sample instructions](../examples/README.md) and [agent control](agent-control.md).

On other platforms provide the Qt prefix through CMake as needed. Linux produces
`build/alloy`. For nonstandard OCCT installs set `OpenCASCADE_DIR` to the folder
containing `OpenCASCADEConfig.cmake`. OCCT is mandatory; initialization errors
have no raster fallback. Alloy uses the public Qt Windows widget style with its
own palette and styling.

The corrected native matrix compiled Windows, Linux and both macOS architectures
at source 12fe534. Each later source candidate needs its own result. Runtime
behavior is verified only on local Apple silicon macOS; hosted compile artifacts
are unbundled, with no tagged runnable release. See [native baseline](dependencies/native-builds.md)
and [releases](releases.md) for pins, packaging and remaining qualification.

## Source map

- `src/main.cpp`: native entrypoint, private endpoint and instance discovery.
- `src/AlloyWindow.{h,cpp}`: shared commands, dialogs, Browser/Timeline, selection
  and actual scene/chrome capture.
- `src/CadDocument.{h,cpp}`: authoritative authored JSON, validation, one
  QUndoStack, paired evaluated snapshots and atomic native storage.
- `src/GeometryKernel.{h,cpp}`: thin OCCT primitive evaluation, validity,
  topology, bounds, area/volume and explicit unit/tolerance policy.
- `src/ReferenceView.{h,cpp}`: native AIS/V3d/OpenGL view, real document BReps,
  disposable preview, body picking, named cameras and driver diagnostics.
- `src/ControlServer.{h,cpp}` and `tools/alloyctl.py`: local JSON transport and
  stdlib client; Windows transport adaptation remains open.
- `tests/primitives_api.py`: real create/edit/pick/undo/save/reopen workflow.
- `tests/shell_api.py`, `document_api.py`, `session_api.py`: earlier app workflows.
- `tests/workflows/run.py`: candidate-bound cumulative runner and fixture ledger.

Authored intent is independent of evaluated OCCT shapes and AIS presentation.
Native files persist authored features, IDs and canonical parameters; reopening
evaluates them and starts a clean empty session journal. No display cache is
treated as the editable document.

## Verification

Use focused core tests and one growing real-entrypoint regression:

```sh
ctest --test-dir build -R '^(document_model|geometry_kernel|client_selection)$' --output-on-failure
python3 tests/workflows/run.py --case primitive-regression --evidence build/evidence/primitive-regression
```

A desktop session is required. The runner launches fresh native apps and invokes
their shared commands; it never substitutes an independent automation model.
`--build` also builds the known configured app target. Without it, compilation
is marked unperformed and the tested prebuilt binary is identified separately.

Observed 2026-10-11 on macOS 26.5.1 arm64, Apple Clang 17.0.0, Python 3.14.5,
Qt 6.11.2 and OCCT 7.9.3:

| Evidence | Result and boundary |
| --- | --- |
| Native build | Debug application compiled and linked with warning flags |
| Focused core tests | Geometry kernel, authored document and client selection passed 3/3 |
| Fresh app regression | Shell, authored units, shared sessions and primitives passed; the aggregate case reuses these four runs |
| Geometry | Valid real solids, analytic volumes/bounds, stable IDs across edits/undo/reopen, invalid input refusal |
| GPU | Live Apple M4 hardware OpenGL; actual BRep shading/picking and distinct isometric/top framebuffer captures |
| Native storage | Atomic authored save, failure preservation, malformed/future-version refusal, editable reopen and saved-checkpoint behavior |
| Physical UI | Toolbar box creation and width typing; Cmd+Z/Cmd+Shift+Z; native Save; Timeline edit and Cancel |
| Independent review | Astra approved the bounded DOC-003/004/005/007/008 slice after two material fixes |
| Remaining acceptance | Face/edge selection, STEP, printing export, full file-dialog/decision coverage, other-OS runtime and visual familiarity remain open |

The [primitive receipt](validation/primitives.md) retains public candidate/fixture
metadata and an Alloy capture. Detailed logs, transcripts and app captures remain
under ignored `build/evidence/primitive-regression/` and
`build/evidence/physical-primitives/`. Earlier [GPU foundation](validation/gpu-foundation.md)
and [shell](validation/neutral-shell.md) receipts preserve their narrower evidence.

Stop after reasonable assurance. Add useful unit tests for core contracts and
pragmatic integration coverage for each new workflow; do not duplicate the same
application sequence under every issue alias.

## GPU and compatibility boundaries

`app.state.viewport.gpu` reports actual OCCT context/vendor/device/version,
initialization errors, native drawable size, device scale and framebuffer pixels.
Known software renderers are rejected for qualification; unidentified drivers
remain unverified. Hardware identity is evidence of acceleration, not a benchmark
or proof of future model performance. The regression checks native startup/resize
dimensions before offscreen captures, avoiding a capture that masks a drawable bug.

`capture.screenshot` combines actual Qt chrome and OCCT `ToPixMap` GPU rendering
of the same scene/camera. It excludes OS decorations, menus and separate dialogs;
`capture.dialog` captures the active real widget dialog. Native compositor and
physical inputs are checked separately using computer-use tools. Never infer
geometry, physical interaction or visual compatibility from a screenshot alone.

Named orthographic cameras, pan/zoom and body picking exist. Arbitrary orbit,
face/edge picking, snapping, an interactive view cube and broader Browser controls
remain pending. Primitive world-XY placement/New Body/default values are bounded
Alloy proposals. Public references are unmatched in size, scale, OS and design
state; no exact visual/interaction match is claimed. GATE-010 requires additional
STEP and cumulative evidence and remains open.

The public [Qt Widgets](https://doc.qt.io/qt-6/qtwidgets-index.html),
[QLocalServer](https://doc.qt.io/qt-6/qlocalserver.html) and
[OCCT visualization](https://www.occt3d.com/dev/doc/overview/html/occt_user_guides__visualization.html)
guides support the foundations (retrieved 2026-10-11). Native scaling was checked
against version-pinned public
[Cocoa_Window](https://github.com/Open-Cascade-SAS/OCCT/blob/V7_9_3/src/Cocoa/Cocoa_Window.mm)
and [OpenGl_Window](https://github.com/Open-Cascade-SAS/OCCT/blob/V7_9_3/src/OpenGl/OpenGl_Window_1.mm)
sources. No proprietary reference application was operated or inspected.
