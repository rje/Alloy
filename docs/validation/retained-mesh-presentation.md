# Retained mesh presentation

XCH-018/#727, 2026-10-21. Bounded implementation with passing execution receipts;
independent source/evidence review accepts the bounded implementation.
Native chooser and import preview remain
XCH-019; cumulative native import acceptance remains XCH-016. The dated external
visual contract is `knowledge/specs/xch016-native-mesh-visual-2026-10-21.md`
in `../Alloy-reference-records/` (REF-X016-01–06).

## Command and presentation contract

`document.importStl {path, units}` retains the accepted XCH-015 adapter and now
selects its stable UUID and fits visible geometry. The Browser has a separate
**Mesh Bodies** category, named source-file rows, an original triangle-network
icon, and visibility checkboxes. Native Browser/canvas selection and
`selection.set {bodyId: meshUuid}` use the same body-members authority. The
selected primary mesh is explicitly reported as `kind: mesh`, `meshId`, and
`bodyId`; it never enters evaluated B-rep bodies. `view.projectBody` accepts a
visible mesh UUID for the real bounds-center projection. That center is a
projection aid, not a promise that arbitrary nonconvex meshes contain it.

`mesh.visibility {meshId, visible}` changes one authored optional Boolean in one
ordinary document transaction. Older mesh records with no field mean visible.
Hide clears selection of that mesh; Undo/Redo and save/reopen retain visibility
and UUID. Active command/rollback refuses visibility changes. Exact vertex and
triangle arrays, placement and source diagnostics stay unchanged. A retained
mesh is independent of the source after import. It has no feature recipe or
solid-face/edge editing menu; the canvas context offers Hide Mesh and Fit.

OCCT GPU primitive arrays draw the actual retained facets with flat normals and
dark triangle edges. A whole-object sensitive triangulation provides accelerated
triangle picking. No display B-rep, fabricated topology or solid is created.
Mesh-specific diagnostic actors report stable UUID, displayed state, facet count
and `brep: false`; ordinary renderer diagnostics remain independently reported.
Fit includes visible retained bounds together with solids, sheets and sketches.

## Bounded verification

The focused adapter check passes **1/1 (0.57s)** and covers old visible records, valid
hidden records and refusal of non-Boolean visibility. The native Qt fixture
uses the real AlloyWindow command entrypoint and actual-hit widgets, with one
mixed closed-mesh/solid document. It observes window activation and post-input
focus/selection without repairs between demonstrated inputs. The local API
fixture also covers an open sheet, import Undo/Redo, hidden source-independent
reopen, exact retained geometry and refusal of solid-face picking on a mesh.

```sh
cmake --build build-preview-release --target alloy retained_stl_import_test \
  retained_mesh_presentation_controls_test -j4
ctest --test-dir build-preview-release -R '^retained_stl_import$' --output-on-failure
python3 tests/retained_mesh_presentation_api.py \
  --app build-preview-release/alloy.app/Contents/MacOS/alloy \
  --output ../Alloy-reference-records/validation/xch018-retained-mesh-2026-10-21/api \
  --fixtures ../Alloy-reference-records/validation/xch014-mesh-reader-2026-10-21/fixtures
```

Native fixture requires explicit `ALLOY_MESH_EVIDENCE` and
`ALLOY_MESH_FIXTURES` directories. Run it separately with effective DPR1 and
DPR2; do not launch alongside another private editor. The accepted local SDK is
Qt6_DIR `/Users/rje/src/cpp/Alloy-dependencies/qt-private-runtime/install/lib/cmake/Qt6`
and OpenCASCADE_DIR `/opt/homebrew/lib/cmake/opencascade`.

## Execution receipts

External evidence is `../Alloy-reference-records/validation/xch018-retained-mesh-2026-10-21/`.
The final Release application and fixture build pass using the SDK above. The
real-entrypoint API fixture passes **110 responses**, including the original
12-facet closed cube and 2-facet open square, one independent solid, import
Undo/Redo, hidden-state history/reopen after source deletion, exact retained
arrays, solid-face refusal on the separated mesh, and both mesh-front and
solid-front overlapping top views. Auto resolves the front mesh through OCCT's
depth-ordered hit before the B-rep-only analytic-face fallback; a solid in front
keeps the existing face-first Auto behavior.

The actual-hit Qt fixture passes **3/3 at DPR2 (5.445s)** and **3/3 at effective
DPR1 (4.523s)**, recording 13 state/native steps per run. It covers import
selection, blank-canvas clear, Browser select/hide/show, Undo/Redo, native mesh
and solid picks, resize/pick and source-independent reopen. An active Box
command refuses an actual Browser checkbox hide; authored state and the visible
actor stay unchanged, and the checkbox restores the authored value under
`QSignalBlocker`. Initial harness activation and collapsing Named Views to expose
the test row occur before demonstrated input; no focus/selection repair occurs
between those steps. Reopen and history are API-driven observations separate
from native clicks. Qt input is not a physical device-input qualification.

Both runs identify **Apple / Apple M4 / OpenGL 4.1 Metal - 90.5**, GLSL 4.10,
MSAA 4, hardware-driver-identified acceleration, on macOS 26.5.1 arm64,
Qt 6.11.2 and OCCT 7.9.3. The main window starts at 1280×900 logical and resizes
to 1360×840. DPR2 has 1028×608 logical / 2056×1216 framebuffer initially and
1108×548 / 2216×1096 after resize. Effective DPR1 (`QT_SCALE_FACTOR=0.5` on
the native DPR2 display) has the same logical dimensions and 1028×608 then
1108×548 framebuffer; native window coordinates are 514×304 then 554×274.
Picking uses the established native-coordinate conversion, separately from FBO
pixel scale. `summary.json` contains the exact dimensions and native model hashes.

The implementer inspected closed/open isometric mixed captures, both overlapping
top-view captures, native selected/hidden/reopened captures and raw resized GPU
frames at DPR1/2. Facets have a deliberate blue selected fill and readable dark
triangle edges; the independent solid retains ordinary solid shading. Browser
labels, triangle icon, check state and selection cue are readable. The screenshot
API composites the native OCCT framebuffer; native fixtures additionally save
raw FBO images. The initial `api/` attempt used an unregistered `capture.viewport`
command and is superseded by `api-final/`; this was a fixture error, not an app
failure. Private editors exited; user-owned PID72618 was preserved. No paid CI,
tag, release, push, proprietary application or extracted asset was used.

Independent source review found a visibility-checkbox refusal mismatch; the
restoration above and its focused native case correct it. A final source audit
also preserves original member order when pruning mixed mesh/solid selection
after a document revision; its focused API regression is recorded separately
from the unchanged native input/renderer evidence. The final-order Release
rebuild and `api-order-final/` 110-response replay pass; the original
`api-final/` 96-response run is retained as the pre-order baseline. The final
check confirms mesh→solid membership and primary selection survive an authored
visibility transaction without reordering. Native input and renderer behavior
are unchanged by this pruning correction and were not redundantly rerun.
`binary-sha256-native-accepted.txt` identifies those native binaries separately
from the final-order binaries. Final review evidence
is available in `api-order-final/transcript.json`, `native-dpr1/report.json`,
`native-dpr2/report.json`, logs and inspected PNGs; the root records its decision.

## Compatibility boundary

| Evidence | Result |
|---|---|
| REF-X016-01–04, committed mesh in canvas and Browser | Matched bounded structure: retained mesh appears in canvas and a named Browser row, including reopen. |
| Faceted shaded surface and triangle edges | Observed Alloy facets and triangle edges; no matched public viewport, theme, camera or source geometry. Exact appearance remains unknown. |
| Browser row name/icon/folder, selection tint and fit defaults | Unknown reference semantics. Alloy's explicit category and independent icon are local proposals. |
| Mesh Base Feature Timeline | Deliberate mismatch: Alloy retains root mesh records with ordinary document Undo/Redo, rather than a fabricated editable solid feature. |
| Native chooser, units/placement preview, OK/Cancel | XCH-019. No acceptance by this presentation prerequisite. |

Physical mouse/keyboard hardware and Windows/Linux packaged runtime are separate
qualification gates. The renderer remains the current accelerated OCCT/OpenGL
prototype; native Metal/Vulkan migration remains decision 003. Capacity GPU
performance, self intersections, watertightness and mesh editing are not claimed.
