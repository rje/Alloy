# Development and observed evidence

Alloy is a C++20, Qt 6.11.2 and OCCT 7.9.3 application with shared UI/API
commands and a hardware-accelerated native viewport. See the
[development preview guide](development-preview.md) for current modeling,
assembly, surface and interchange capabilities and their evidence limits.
Authored sketches and features remain separate from evaluated geometry.

## Build and run

Use an optimized build for interactive previews and performance measurements.
Keep Debug builds for debugging; their timings do not represent the optimized
application or its in-tree assembly solver.

On the development Mac, use the coherent patched Qt SDK rather than mixing
Homebrew Qt libraries with a separately built Cocoa plugin. The SDK at
`../Alloy-dependencies/qt-private-runtime/install` is used by the accepted local
workflows. The [private Qt recipe](../tools/dependencies/build-private-qt.sh)
and [recipe evidence](validation/mac-runtime-recipe.md) describe reproduction
and its pinned public dependencies. Preserve the SDK when rebuilding Alloy.

```sh
ALLOY_QT_PREFIX="$PWD/../Alloy-dependencies/qt-private-runtime/install"
cmake -S . -B build-preview-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$ALLOY_QT_PREFIX;/opt/homebrew" \
  -DQt6_DIR="$ALLOY_QT_PREFIX/lib/cmake/Qt6"
cmake --build build-preview-release --target alloy
./build-preview-release/alloy.app/Contents/MacOS/alloy --ready-file /tmp/alloy-ready.json
```

CMake requires the exact Qt/OCCT versions and matching solver headers described
below; install prerequisites before configuring. The existing
`build-tutorial-polish` is the Debug validation tree using the same patched SDK.
Use its focused tests when debugging and identify the build configuration in
receipts. Do not assume a directory named `build` contains the accepted SDK.
The [accessibility investigation](dependencies/qt-cocoa-accessibility.md) retains
earlier plugin experiments as history; the current path uses a coherent private
Qt build. Neither a source build nor a patched plugin is a relocatable package;
see the [package receipt](validation/macos-relocatable-preview.md).

Use File → Open for `examples/gauge-block.alloy`. Click a Timeline feature to
edit its dimensions, or Box/Cylinder to create another body. See
[sample instructions](../examples/README.md) and [agent control](agent-control.md).
`examples/dimensioned-rectangle.alloy` exercises the integrated sketch solver:
re-enter its Timeline feature and double-click a driving label or edit it in the
palette. D places line dimensions with an adjacent numeric editor.
`examples/sketch-extrusion.alloy` adds an editable 5 mm dependent extrusion.
Finish a closed sketch, select its region and press E; the right dock provides
live signed-distance preview and OK/Cancel. Home fits the preview obliquely.
The consumed sketch hides on commit, can be shown with its Browser checkbox,
and remains editable through its Timeline feature. Source editing temporarily
hides dependent solid presentations; Finish recomputes them, Cancel restores them.

The solver build uses Eigen 5.0.1 and Boost 1.90.0 headers. CMake accepts matching
installed versions or fetches the pinned, hash-checked archives by default;
Homebrew versions may differ. Set `ALLOY_FETCH_SOLVER_HEADERS=OFF` and provide
`ALLOY_EIGEN_INCLUDE_DIR`/`ALLOY_BOOST_INCLUDE_DIR` for matching offline headers.
The first configure also downloads and verifies the pinned PlaneGCS extraction
into `build/_deps/planegcs/`; cached originals are checked on later configures.
See [decision 004](decisions/004-sketch-solver.md) and the
[dependency inventory](dependencies/README.md) for licenses and packaging limits.

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
- `src/GeometryKernel.{h,cpp}`: ordered sketch/profile/primitive/extrusion evaluation, validity,
  topology, bounds, area/volume and explicit unit/tolerance policy.
- `src/SketchGeometry.{h,cpp}`: authored points/lines/relations/dimensions, strict
  references, solved draft mutations, origin-plane frames and real OCCT edges.
- `src/SketchProfiles.{h,cpp}`: stable single-loop profile derivation, actual valid
  planar faces, area and source-edge provenance; unsupported boundaries keep diagnostics.
- `src/SketchSolver.{h,cpp}`: MIT adapter, transient PlaneGCS storage, normalized
  coordinates, diagnostics and independent residual checks.
- `cmake/PlaneGCS.cmake`, `third_party/planegcs/`: pinned shared-library build,
  source manifest, portability shims and retained third-party notices.
- `src/ReferenceView.{h,cpp}`: native AIS/V3d/OpenGL view, real document BReps,
  disposable preview, body picking, named cameras and driver diagnostics.
- `src/ControlServer.{h,cpp}` and `tools/alloyctl.py`: local JSON transport and
  stdlib client; Unix sockets and bounded Windows named-pipe transport.
- `tests/primitives_api.py`: real create/edit/pick/undo/save/reopen workflow.
- `tests/sketch_api.py`, `tests/sketch_document.cpp`: sketch drawing, identity,
  solved mutations, failure preservation, history and storage contracts.
- `tests/sketch_solver.cpp`: supported adapter relations, dimensions and failure diagnostics.
- `tests/sketch_constraints_api.py`: actual constrained rectangle, dimension
  editing, selection, solve failure, undo/cancel and save/reopen workflow.
- `tests/sketch_annotations_api.py`, `tests/sketch_profiles.cpp`: placed/edited
  dimensions, exact placement-only mutation, real face derivation and profile picking.
- `tests/sketch_extrude_api.py`, `tests/extrusion_document.cpp`: finished region selection,
  signed prism geometry, upstream edits, candidate failure preservation and paired
  history/storage; the real app uses the same session and document authority.
- `tests/shell_api.py`, `document_api.py`, `session_api.py`: earlier app workflows.
- `tests/workflows/run.py`: candidate-bound cumulative runner and fixture ledger.

Authored intent is independent of evaluated OCCT shapes and AIS presentation.
Native files persist authored features, IDs and canonical parameters; reopening
evaluates them and starts a clean empty session journal. No display cache is
treated as the editable document.

## Verification

Use focused core tests and one growing real-entrypoint regression:

```sh
ctest --test-dir build-tutorial-polish -R '^(document_model|geometry_kernel|sketch_document|sketch_solver|sketch_profiles|extrusion_document)$' --output-on-failure
python3 tests/workflows/run.py --case sketch-extrude \
  --app build-tutorial-polish/alloy.app/Contents/MacOS/alloy \
  --evidence ../Alloy-reference-records/deliverables/sketch-extrude-current
```

A desktop session is required. The runner launches fresh native apps and invokes
their shared commands; it never substitutes an independent automation model.
Build the intended target explicitly before running. The runner’s legacy
`--build` option targets `build/`, so omit it for the named build trees above;
the runner records the prebuilt binary separately from compilation evidence.

Earlier primitive candidate observations, recorded 2026-10-11 on macOS 26.5.1
arm64, Apple Clang 17.0.0, Python 3.14.5,
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
The later [viewport mouse regression](validation/viewport-input.md) records the
current fix, fresh cumulative regression and native mouse checks. Its retained
report is `build/evidence/viewport-fix-final/report.json`.

The [extrusion receipt](validation/sketch-extrusion.md) records the later ordered
sketch/profile/solid chain and its cumulative `sketch-extrusion-checkpoint` milestone.
The real-app case verifies 40 × 25 × 5 mm = 5000 mm³, width 50 → 6250 mm³,
stable profile/body/feature IDs, invalid input preservation, one-entry commits,
undo/redo and reopen. Separate native input covers drawing/selection, E, distance
entry, upstream label editing and solid restoration. Use `--case sketch-extrude`
for focused changes; reserve `--case sketch-extrusion-checkpoint` for the coherent
milestone, rather than rerunning unchanged workflows for every adjustment.

Stop after reasonable assurance. Add useful unit tests for core contracts and
pragmatic integration coverage for each new workflow; do not duplicate the same
application sequence under every issue alias.

The later [sketch receipt](validation/sketches.md) records the integrated origin-plane
line/rectangle slice, three focused core tests, native drawing and retained state.
Its earlier app cases passed on the same binary; only the new sketch case was
rerun after correcting test expectations. For a sketch-only change use `--case sketches`
when existing case evidence on the unchanged application candidate remains applicable.
The [constraints receipt](validation/sketch-constraints.md) records the later
integrated adapter/document tests and actual application workflow. It distinguishes
the cumulative predecessor from focused reruns after material fixes. That slice
adds automatic rectangle H/V, anchored width/height edits, a driven diagonal,
normalized OCCT edges, failed-solve draft preservation and authored save/reopen.
For a constraints-only change, use `--case sketch-constraints` when the earlier
case evidence remains applicable; the `constraint-regression` aggregate includes
the earlier sketch, primitive and shell workflows without duplicating aliases.

## GPU and compatibility boundaries

`app.state.viewport.gpu` reports actual OCCT context/vendor/device/version,
initialization errors, native drawable size, device scale and framebuffer pixels.
Known software renderers are rejected for qualification; unidentified drivers
remain unverified. `gpu.msaa` reports requested, capability-approved enabled and
maximum samples plus texture multisampling support. The extrusion candidate
selects 4× on Apple M4 (maximum 4); this records the rendering configuration,
not a separate driver framebuffer-sample measurement. Dark edges, an opaque neutral
prism preview and a fixed world-space key plus ambient light make oblique faces
readable; sketch/profile fill remains unlit. Hardware identity is evidence of acceleration,
not a benchmark or proof of future model performance. The regression checks native startup/resize
dimensions before offscreen captures, avoiding a capture that masks a drawable bug.

`capture.screenshot` combines actual Qt chrome and OCCT `ToPixMap` GPU rendering
of the same scene/camera. It excludes OS decorations, menus and separate dialogs;
`capture.dialog` captures the active real widget dialog. Native compositor and
physical inputs are checked separately using computer-use tools. Never infer
geometry, physical interaction or visual compatibility from a screenshot alone.

Named orthographic cameras, pan/zoom, body picking and active-sketch point/line
picking exist. Orange marks selected sketch geometry, green marks fixed points,
and black/blue reports whole-sketch zero/nonzero DOF. These are not per-entity
constraint diagnoses. Arbitrary orbit, solid face/edge picking, snapping, an
interactive view cube and broader Browser controls
remain pending. Finished sketch profiles are filled/selectable; body/sketch Browser
checkboxes invoke actual undoable visibility commands. Primitive world-XY
placement/New Body/default values are bounded
Alloy proposals. Public references are unmatched in size, scale, OS and design
state; no exact visual/interaction match is claimed. GATE-010 requires additional
STEP and cumulative evidence and remains open.
The later [annotation/profile receipt](validation/sketch-annotations.md) records
placed dimensions, numeric typing, double-click editing, annotation dragging and
single-loop face/picking acceptance. For a change to that slice, use the focused
`sketch_document`/`sketch_profiles` CTest checks and `--case sketch-annotations`.
The first extrusion supports one accepted simple closed line loop on XY/XZ/YZ,
signed magnitude 0.001–100000 mm, Profile Plane / One Side / Distance / New Body.
Its distance field uses current display units; typed unit suffixes, Flip button,
interactive handle, expressions, booleans and other extent/operation modes are
not implemented. API `units` conversion exists. Native point dragging, inline
rectangle entry and wider inference remain open.
Their specific interaction and per-entity badge/visibility follow-ups are recorded in
[sketch controls](research/commands/sketch-controls.md).

The public [Qt Widgets](https://doc.qt.io/qt-6/qtwidgets-index.html),
[QLocalServer](https://doc.qt.io/qt-6/qlocalserver.html) and
[OCCT visualization](https://www.occt3d.com/dev/doc/overview/html/occt_user_guides__visualization.html)
guides support the foundations (retrieved 2026-10-11). Native scaling was checked
against version-pinned public
[Cocoa_Window](https://github.com/Open-Cascade-SAS/OCCT/blob/V7_9_3/src/Cocoa/Cocoa_Window.mm)
and [OpenGl_Window](https://github.com/Open-Cascade-SAS/OCCT/blob/V7_9_3/src/OpenGl/OpenGl_Window_1.mm)
sources. No proprietary reference application was operated or inspected.
