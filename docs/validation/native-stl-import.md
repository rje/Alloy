# Native STL import

XCH-019/#728, 2026-10-21. Bounded product/API/ready-panel execution passes. Strict OS chooser routing and
popup readiness remain XCH-022/#731; the root records independent acceptance separately.
The permitted public workflow contract is external at
`../Alloy-reference-records/knowledge/specs/xch016-native-mesh-visual-2026-10-21.md`
(REF-X016-01–06). XCH-015 owns strict retained geometry; XCH-018 owns committed
Browser/GPU/selection. XCH-016 remains cumulative native acceptance.

## Bounded command contract

**File > Import Mesh…** and command discovery expose
`document.importMeshDialog`: one native local STL chooser. Chooser Cancel has no
model effect. `mesh.importPreview {path}` prepares the same retained mesh adapter
at explicit default mm; a right-side **Import Mesh** panel offers **Unit Type**
(Millimeter/mm or Inch/in), source filename, triangle/vertex count, live canonical
mm min/max bounds, GPU facets, OK and Cancel. The ready preview is not an authored
mesh, B-rep, solid, Browser row, or undo entry. Source XYZ is preserved.
`mesh.importUnits {units}` derives every unit change from the original prepared
coordinates, avoiding accumulated round trips. `app.state.meshImport` exposes
inputs, preview summary and nonauthored state. Model and selection changes are
refused while the chooser or ready preview owns the workflow.

`mesh.importAccept` invokes the authoritative `document.importStl` transaction
with the internally prepared candidate, retaining its exact validated bytes even
if the local source changes or disappears. The public direct import path retains
its original explicit-path/units contract. No API accepts arbitrary prepared
geometry. OK creates one named, selected, visible retained mesh, fits the view,
and gives one Undo with stable-ID Redo; existing XCH-018 persistence applies.
`mesh.importCancel` and panel Cancel/Escape discard the ready preview. The
preview fit changes view framing; document, dirty/history, saved bytes and body
selection remain unchanged. Invalid input reports filename, reason and retry
remedy in a native warning; no panel or authored mesh is left behind.

## Loading boundary

STL reading/decoding, scaling/quality validation and final document validation
remain synchronous UI-thread operations. The panel appears after initial loading;
there is no responsive loading screen, progress or in-flight Cancel. Cancel is
available on the **ready** preview. XCH-021 preserves asynchronous
loading/progress/cancel, and remains a prerequisite for cumulative native mesh
acceptance. The prior 200000-facet import measured 3.449 seconds; this candidate
does not claim to resolve that pause. OK reuses prepared geometry rather than
rereading the source, but still validates and commits synchronously.

## Verification and evidence

The final Release app and native fixture build pass. The focused retained STL
adapter CTest passes **1/1 (0.60 s)**. Local build uses `build-preview-release`, Qt6_DIR
`/Users/rje/src/cpp/Alloy-dependencies/qt-private-runtime/install/lib/cmake/Qt6`
and OpenCASCADE_DIR `/opt/homebrew/lib/cmake/opencascade`, `-j4`.

```sh
cmake --build build-preview-release --target alloy native_stl_import_controls_test retained_stl_import_test -j4
ctest --test-dir build-preview-release -R '^retained_stl_import$' --output-on-failure
python3 tests/native_stl_import_api.py \
  --app build-preview-release/alloy.app/Contents/MacOS/alloy \
  --fixtures ../Alloy-reference-records/validation/xch014-mesh-reader-2026-10-21/fixtures \
  --output ../Alloy-reference-records/validation/xch019-native-stl-2026-10-21/api
```

The GUI fixture requires explicit `ALLOY_MESH_EVIDENCE` and `ALLOY_MESH_FIXTURES`;
run effective DPR1 and DPR2 separately under the single private-editor lease.
Run `native_stl_import_controls_test readyPanel` for the bounded accepted Qt
panel fixture: it prepares through the shared API and uses actual-hit panel and
Browser controls without focus/selection repairs. Initial harness activation is
separate. The `chooserRouting` case is a separate, incomplete XCH-022 probe: it
verifies the fixture PID is frontmost before any OS key, aborts safely otherwise,
and deliberately stops before unqualified source-selection popup input. It does
not repair focus or send input to a user-owned app. Neither route establishes
physical hardware qualification.

## Execution receipts

Evidence is external under
`../Alloy-reference-records/validation/xch019-native-stl-2026-10-21/`.
`api-final/` passes **84 responses** on the final application binary. It checks
closed and open originals, exact 25.4-scaled bounds and every source-oriented
triangle coordinate, unchanged authored/history/dirty/selection and saved bytes
on preview Cancel, invalid-input atomicity, Undo/Redo, source deletion before OK,
and source-independent native save/reopen. All eight authored named-view,
Home and view-convention mutations refuse while previewing. A base document
revision check independently prevents stale prepared-candidate publication.

The final `ready-dpr2/` fixture passes **3/3 (5.000 s)**. It prepares the ready preview
through the shared API, then delivers Qt input to actual-hit controls. It covers
API/native unit-field synchronization, refusal/recovery of invalid units,
10 mm → 254 mm bounds via native units input, ready-preview Cancel preserving
document/selection/saved bytes, OK selecting the committed mesh, native Browser
collapse and checkbox hide before the open-sheet preview, resize and OK.
A source-file failure through the shared API remains atomic. This fixture
exercises the product's ready-panel behavior without OS file-selection input.

The final `ready-dpr1/` replay stopped at the editor AppKit key-window assertion
after its first API-prepared preview (**2 pass / 1 fail, 18.056 s**); no repair or
OS key followed. Its directory retains earlier successful ready-panel snapshots
from before the queued Browser-hide observation was strengthened; those stale
`report.json`/model snapshots are not evidence that the final replay passed.
The geometry checker excludes that directory. This final focus/ownership gate
remains open under XCH-022. DPR1 panel/GPU observations below remain bounded
observations, not a strict end-to-end replay acceptance.

Earlier `native-dpr2/` and `native-dpr1/` runs reported **3/3 (16.332 s)** and
**3/3 (14.975 s)**, with 16 observed states each. They observed NSOpenPanel key
ownership, chooser Escape and source-file selection/return, Unit Type focus,
ready-panel controls and an actionable native invalid-file warning/dismissal.
Those runs used global OS keys and fixed delays inside Command-Shift-G navigation;
they did not establish the transient Go To overlay's actual readiness. A later
PID-targeted guarded probe found a mismatch between System Events' frontmost PID
and local AppKit active/key-window observations and safely stopped before sending
its first key. Therefore those earlier runs are **observed, not strict native
chooser acceptance**. Their GPU captures and saved geometry remain independently
useful. XCH-022/#731 owns strict PID-targeted OS routing, chooser Cancel/selection,
Go To activation/focus/closure and uninterrupted return, and is a prerequisite
of cumulative XCH-016. No full OS chooser routing/focus acceptance is claimed.
The final source has no fixed-delay Go To input path; `chooserRouting` fails
safely when ownership differs and stops before unqualified source selection.

The fixtures write `native-imported.alloy` and geometry-page transcripts.
External `check-native-geometry.py` independently parses both source STL files
and compares every saved oriented triangle in each DPR run. The qualified geometry snapshots retain exact
mm coordinates/order/winding: closed cube 12 facets/8 vertices/0 boundary edges,
open square 2 facets/4 vertices/4 boundary edges. No repair or solid conversion
occurs. `native-geometry-summary.json` records this separate geometry check;
API history/source-independent-reopen evidence is separate from native input.

The observed GPU runs and final DPR2 ready-panel run identify **Apple / Apple M4 / OpenGL 4.1 Metal - 90.5**,
GLSL 4.10, MSAA 4 and hardware-driver-identified acceleration on macOS 26.5.1
arm64, Qt 6.11.2 and OCCT 7.9.3. Preview window size starts 1280×900 logical,
resizes to 1360×840; the dock leaves a 714×608 then 794×548 logical viewport.
DPR2 framebuffers are 1428×1216 then 1588×1096; effective DPR1
(`QT_SCALE_FACTOR=0.5` on the native DPR2 display) is 714×608 then 794×548.
Native-coordinate viewport sizes differ at effective DPR1 (357×304 then
397×274); actual-hit Qt delivery uses native coordinate conversion separately
from framebuffer size. The closed committed state, after dock closure, has the
wider XCH-018 viewport. The summary records exact per-state sizes.

The implementer inspected observed full-window selected/preview/error captures and
raw GPU frames at both scales, including the resized open preview. Source names,
unit labels, complete bounds/count and bottom actions are readable after layout
settles. Blue unlit preview facets and dark triangle edges are distinct from
the committed flat-normal shaded mesh and ordinary solid. The original API
capture before the unit-field synchronization correction is superseded; a native
capture taken before queued label relayout is likewise superseded by the fixture's
observed size readiness. No exact public material/shading parity is claimed.
`binaries-sha256.txt` identifies final app and ready-fixture binaries;
`binaries-observed-sha256.txt` and `native-controls-observed.cpp` preserve the
earlier methodology separately. Do not run that superseded global-key fixture.

Independent architecture review found two material corrections: shared API units
must update visible controls/error/OK together, and authored view mutations must
not be admitted while a whole-document candidate is prepared. Centralized
`QSignalBlocker` control synchronization, explicit view-mutation exclusions and
base revision validation correct them, with focused API/native regressions.
Native probes found absent AppKit key return after chooser/dock disposal; the
product lifecycle restoration above fixes it. An earlier fixture selected the
wrong File widget type and continued after failure; corrected actual-hit
QPushButton delivery and fail-stop handling replace that invalid probe. Final
private editor processes exited; user-owned PID72618 was preserved.

## Compatibility comparison

| Public evidence | Bounded comparison |
|---|---|
| REF-X016-01–04 local choose, units, preview, OK and canvas/Browser mesh | Matched bounded units/ready-preview/OK/canvas/Browser structure; OS chooser operation is observed but strict routing/readiness remains XCH-022. |
| Entry label/location | Deliberate mismatch: File > Import Mesh rather than the contextual insertion route. |
| Right-side settings and central faceted preview, Browser at left | Relative arrangement implemented; no matched viewport/theme/camera/reference geometry, so exact styling is unknown. |
| Units | mm/in explicit interpretation; cm/m/ft deferred. Default-mm and persistence parity unknown. |
| Placement | Deliberate reduced scope: source XYZ identity; no manipulator, center/ground, flip, XYZ angles or Reset. |
| Batch/other formats | One STL only; OBJ XCH-017, broader batch/3MF separate. |
| Timeline | Root retained mesh with ordinary document history; no Mesh Base Feature parity claimed. |
| Cancellation, errors and focus keys | Ready-panel Cancel and shared invalid-file atomicity pass; OS chooser and its popup routing/readiness remain XCH-022. Public exact semantics remain unknown. |

No proprietary application, copied asset, paid CI, tag, push or release is used.
Physical input and Windows/Linux packaged runtime remain separate gates. The
viewport remains the accelerated OCCT/OpenGL prototype under decision 003.
