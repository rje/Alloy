# Responsive STL import

XCH-021/#730, 2026-10-21. Responsive core/API and bounded DPR2 actual-hit
execution pass. Strict DPR1 focus/input remains XCH-022/#731. Native STL
preparation now starts an immediate
**Loading mesh…** panel with phase/progress and an enabled Cancel. The bounded
implementation uses the existing retained STL and shared `document.importStl`
authority. Strict OS chooser routing remains XCH-022/#731; this receipt covers
API preparation and actual-hit Qt panel input, not native file selection.

## Command and lifetime contract

`mesh.importPreview {path}` returns `status: pending` after opening the panel.
Poll `app.state.meshImport.status` for `pending`, `ready` or `failed`; inspect
`phase`, `completed` and `total` while pending. A zero total means indeterminate
work, including the opaque OCCT decoder. The panel shows the filename and an
indeterminate bar immediately, then bounded counters where available. Units and
OK remain disabled during preparation; Cancel/Escape remain enabled. The summary
reserves enough text height for changing phases before its first paint.

One worker owns preflight, staged-byte OCCT decoding, exact-coordinate retention,
quality analysis, unit scaling, document structural validation and expansion of
preview facets. It captures owned input/document snapshots and never accesses
widgets or dereferences the live document. Atomics publish progress and an
acquire/release terminal result. Cancellation checks bracket OCCT and occur in
bounded parser/retention/preview loops. OCCT and a quality/validation phase may
finish before cancellation is noticed; their result is discarded. Cancel does
not promise to interrupt an opaque kernel operation.

Cancel invalidates the session generation before disposing the panel and clears
preview/token state. A weak draining-job lease prevents another worker starting
until the cancelled job finishes; an early retry reports `import_busy` with a
retry instruction. Publication and acceptance check the tab, document identity,
revision and exact authored base. Document/tab navigation cancels the import;
generation-bound queued polls cannot publish into a replacement session. A worker
may safely outlive the window because it holds only its own snapshots/result.

`mesh.importUnits {units: mm|in}` starts another pending preparation from the
owned original mm interpretation. No path is reread, including after deletion.
Each interpretation retains the same UUID and original oriented triangle arrays;
only canonical-mm coordinates, units/scale and derived summary change. API unit
changes while pending are refused before mutating visible/session inputs.

Ready OK invokes `document.importStl` with an internal opaque prepared-append
token. Its sole producer validates the complete candidate off the UI thread and
constructs exactly one appended mesh. Adoption verifies unchanged revision and
exact base, then reuses the existing evaluated B-rep graph because mesh records
are separate from it. No public command accepts prepared JSON. The ordinary
direct `document.importStl {path,units}` path remains synchronous and validated.
The displayed prepared candidate is committed without another parse or quality
pass. One Undo removes it; Redo and source-independent reopen preserve it.
`app.state.meshImportOperation` retains terminal timing/status after closure;
Cancel of pending, ready or failed panels consistently records cancelled/discarded.

## Verification

Accepted build: `build-preview-release`, Qt 6.11.2 from
`/Users/rje/src/cpp/Alloy-dependencies/qt-private-runtime/install/lib/cmake/Qt6`,
OCCT 7.9.3 from `/opt/homebrew/lib/cmake/opencascade`, Release, `-j4`.
The retained STL adapter CTest passes 1/1 (0.54 s). The updated small actual-app
API fixture passes 95 responses at effective DPR1 on the final product app.
An earlier native-scale API pass has 94 responses; only summary-label sizing
changed afterward. The fixture covers closed and open originals, exact oriented coordinates,
25.4 scaling, Cancel atomicity, source deletion before OK, Undo/Redo and reopen.
Its external transcript and summary retain exact response counts.

The explicit GUI fixture is `responsive_stl_import_controls_test`; it is not
registered in automatic CTest. Set `ALLOY_MESH_EVIDENCE`, `ALLOY_MESH_CAPACITY`
(the original 200000-facet source) and `ALLOY_MESH_FIXTURES`. Run effective DPR2
and effective DPR1 (`QT_SCALE_FACTOR=.5`) sequentially under the private-editor
lease. The 5 ms Qt heartbeat threshold was fixed at **2000 ms** before the run,
consistent with the existing bounded responsiveness precedent. Publication and
commit are measured separately, because GPU AIS publication and viewport refresh
still occur on the UI thread. Timing gates cover preparation/unit change/commit;
synchronous save/reopen are outside that heartbeat interval.

The fixture uses actual hit widgets for pending Cancel and OK, verifies the
publication-produced units focus before its next key, and does not repair focus
or selection between those steps. It also cancels on tab/new-document change,
waits through the sole draining lease, deletes the source before native units/OK,
checks one Undo/Redo, reopens and resaves to byte-identical native data, and
refuses truncated/over-cap inputs without authored mutation. Initial harness
activation is separate. This is Qt input evidence, not physical hardware or
strict OS chooser routing.

External evidence is under
`../Alloy-reference-records/validation/xch021-responsive-stl-2026-10-21/`:
`dpr2/` (qualified Qt input), `dpr1/` (unqualified final input), `api/`,
`api-dpr1/` (noninput), original-source geometry checker, native run/RSS sampler
and binary hashes. The qualified DPR2 geometry checker reads every independently authored source
facet and compares the saved oriented arrays, source SHA-256, identity placement,
exact inch scale and quality. Byte-identical reopen saves separately verify all
retained data survived decoding/persistence, rather than relying on summaries.

The final product's DPR2 fixture passes **3/3 (10.189 s)**. All times below
are milliseconds; the original source contains 200000 facets / 600000 vertices.

| Stage | Worker | UI GPU publication | Submit-to-ready wall | External heartbeat max gap |
|---|---:|---:|---:|---:|
| Initial mm preparation | 596 | 39 | 670 | 51 |
| Native inch interpretation after source deletion | 578 | 34 | 625 | 84 |

Pending actual-hit Cancel has a 122 ms observed gap including the requested
screenshot. Shared-command OK takes 990 ms; the full actual-hit OK handler and
panel closure take 1162 ms, with a **1194 ms** largest heartbeat gap. These pass
the preselected 2000 ms gate, while clearly retaining a roughly one-second
synchronous commit/viewport publication interval. Full-sequence sampled peak RSS
is **787120128 bytes**. The source hash is
`de1280a3b9ce21b480f44f999a55c2356fab7a25976cd45fbc05ebe2266b5beb`;
all 200000 oriented triangles and 600000 exact inch-scaled positions pass the
independent checker. Native save/reopen/resave SHA-256 is
`2abe78e7766f943d247d9c007bf826a919c2a65132e17656d7b9a5dd7976315f`.

The final DPR1 full-cap run safely stopped before its units key because focus
was null; no repair followed. A single initial-only PID-targeted activation
retry then stopped before any workflow input: System Events reported frontmost
PID55782 while the private fixture was PID84175. This reproduces the host
ownership disagreement retained by XCH-022. The failed attempt and initial retry
logs/RSS are preserved separately; the failed directory has no passing report.
Earlier DPR1 instrumented runs before the summary-height correction passed
load/unit/Cancel/OK/Undo/reopen checks, but are observations rather than final
strict native input acceptance. The final small API DPR1 run passes without
keyboard/pointer input, and its captures/diagnostics are qualified only as
noninput GPU/API evidence. No further global-key retry was made.

`binaries-dpr2-sha256.txt` identifies the passing DPR2 app and fixture;
`fixture-dpr2-observed.cpp` preserves that fixture's source. The product app is
unchanged in `binaries-current-sha256.txt`; the current fixture adds only the
initial PID/active-window guard that safely rejected the final DPR1 retry.
The corresponding remaining native focus/input gate stays in XCH-022/#731;
XCH-021's bounded acceptance does not close it.

The earlier XCH-015 direct import baseline was 3448.7 ms, with 660242432 bytes
sampled whole-editor peak RSS. These runs include preview, cancellation, a second
unit interpretation, history, native save and reopen; their sampled RSS is a
**full-sequence observation on a different sequence**, not a matched memory
regression comparison. Sampling uses `ps` approximately every 25 ms, not an OS
hard peak. The 512 MiB importer workset estimate remains an admission estimate,
not a bound on whole-process RSS. XCH-020/#729 retains subsequent-modeling
validation/performance work; this change does not claim to fix that path.

## Visual and compatibility boundaries

The implementer inspected qualified DPR2 full-window Loading/ready/units captures
and raw GPU frames, observed DPR1 frames and final noninput API DPR1 captures,
including a 1360×840 resize from 1280×900 logical size.
Labels, phase text, units, bounds/count and bottom actions are readable. The
200000 disconnected source triangles are tiny at fit scale, so their dense
appearance is a sampling observation rather than a claim about broad shading
parity. The GPU identifies Apple / Apple M4 / OpenGL 4.1 Metal - 90.5, GLSL 4.10,
MSAA4 and hardware-driver acceleration on macOS 26.5.1 arm64. Native framebuffer
sizes are recorded with the evidence; the renderer remains decision 003's
accelerated OCCT/OpenGL prototype.

REF-X016-01–06 in the external dated native-mesh contract remain the public
reference record. Bounded units/preview/OK/canvas structure is matched; File >
Import Mesh location, mm/in-only choices and absent placement tools retain the
documented deliberate differences. Exact loading/cancellation/default/focus,
colors and screenshot parity are unknown in the public evidence. These Alloy
captures are not viewport/theme/camera-matched public comparisons.

Independent architecture source review found no material worker-lifetime or
adoption blocker; its terminal-Cancel-state correction is implemented and
asserted by the fixture. The root records final bounded feature review before closure. No proprietary
application, copied reference asset, paid CI, tag, push or release was used.
Windows/Linux runtime and strict chooser qualification remain separate gates.
The user-owned PID72618 was preserved and private test editors exited.
