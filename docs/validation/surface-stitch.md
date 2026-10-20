# Two-sheet retained Stitch

SRF-056/#716 implements the bounded geometry/API ingredient of SRF-006/#65.
SRF-057/#717 retains the actual-hit native and visual workflow gate. General
sheet networks, curved/partial seams, healing choices and closed-solid promotion
remain outside this increment.

The research contract is external:
`../Alloy-reference-records/knowledge/specs/srf006-stitch-2026-10-20.md`
(retrieved 2026-10-20; REF public command documentation and OCCT documentation).
Command location, selected faces, tolerance, seam/free-edge status and shell/solid
outcome are documented. The finite two-rectangle scope, precise gap metric,
threshold policy and refusal diagnostics are Alloy proposals. Matched screenshot
parity remains unknown; no reference application was operated.

## Command contract

`surface.stitch` and Surface > Modify > Stitch (`surface.stitchDialog`) invoke
`CadDocument::surfaceStitchCandidate` and the same ordered OCCT evaluation.
The inputs are two ordered `sourceFaces` references, each exactly
`{bodyId, producerFeatureId, faceId}`, and `toleranceMm` (default 0.01 mm,
allowed 1e-7 through 1 mm). Optional `featureId` edits the retained feature;
`name` changes its label. IDs for the feature, result body and source-derived
faces/edges remain stable on supported edits/reopen.

Each selected local same-owner body must contain one finite valid planar
rectangle. This first slice accepts coplanar rectangles with exactly one full
straight matching seam. It measures the maximum paired endpoint separation of
that full finite seam before sewing, requires no longitudinal mismatch, and
refuses partial, overlapping, competing or excessive-gap seams. Gap is reported
in mm independently of input/output B-rep tolerances. An input entity tolerance
above the declared threshold refuses before sewing. Sewing uses private geometry
copies and an explicit maximum tolerance; output tolerance above the threshold
also refuses.

The successful result must be one valid connected open shell with two faces,
one two-face seam, six outer free edges, seven unique edges and no solids or
multiple edges. The inspectable result reports `sourceFaces`, `sourceBodyIds`,
`requestedToleranceMm`, `measuredSeamGapMm`, `gapUnit`,
`maxInputToleranceMm`, `maxOutputToleranceMm`, `toleranceIncreaseMm`,
`sewnEdgeCount`, `freeEdgeCount`, `shellClassification=open`, and `isSolid=false`.
Both source bodies are consumed only after successful candidate evaluation;
their authored features remain in history. Refusal preserves model, revision,
undo history and evaluated source shapes.

The basic readable dock exposes selection count/Clear, numeric tolerance,
gap/count/tolerance diagnostics, preview, OK and Cancel. It uses the shared
`command.update`, `command.preview`, `command.commit` and `command.cancel`
session authority. Escape is wired; native key/pointer delivery is not accepted
by the API evidence. Canvas picking and exact preview/status presentation remain
SRF-057 acceptance work.

## Verification

Independent Astra read-only review accepted commit `c6cc33e` for the bounded
two-planar-sheet core. It checked source identity and consumption timing, private
OCCT copies, selected-seam adjacency, input/output tolerance limits, retained
history, the 163-call transcript, all 15 evidence-manifest hashes and the actual
sub-tolerance capture. The visible striping remains SRF-057 work.

Final local Release build and focused `surface_stitch` CTest pass (1/1).
The final real-entrypoint fixture passes 163 calls: exact/sub-tolerance success,
excessive-gap atomic refusal, isolated preview/Cancel/commit, retained tolerance
edit, source Height edit from 400 to 420 mm² with same result/face IDs,
Undo/Redo and save/reopen. The C++ fixture additionally checks source-position
recompute and independent B-rep adjacency, area/bounds and input-tolerance guards.
The selected source boundaries are explicitly required to be the actual shared
seam after sewing.

External evidence:
`../Alloy-reference-records/validation/surface-stitch-2026-10-20/` contains
`build.log`, `preview-stage-build.log`, `core-test.log`, `result.json`,
`transcript.json`, source/result `.alloy` files and `evidence-manifest.json` with
SHA-256 hashes. `initial-preview-overlap/` preserves the first API pass whose
capture exposed coincident source-body preview actors. Candidate staging now
hides consumed source actors, replaces the candidate body and survives view.home.
The final fixture asserts that stage/replacement state.

Inspected final whole-app GPU captures are `preview-gap-0.png`,
`preview-gap-0.005.png` and `reopened-gap-0.005.png` (2560 x 1800 pixels).
Apple vendor / Apple M4 renderer / OpenGL 4.1 Metal - 90.5 / effective DPR2
are reported by the actual viewport; the dock-open native viewport is
728 x 608 logical pixels with a 1456 x 1216 framebuffer. The exact preview is
clear, with readable tolerance/gap/count labels. The sub-tolerance preview and
reopen captures retain limited coplanar fill striping with source sketches visible;
a source-sketch/profile overlay is an inference, not an isolated cause. This is
an explicit **visual mismatch remaining under SRF-057**, not accepted visual parity.
No native actual-hit input was delivered or claimed. Final private PID19875
exited cleanly; user-owned PID72618 was preserved.

Observed exact-case maximum input/output B-rep tolerance is 0.000001 mm with no
increase. At measured gap 0.005 mm, output maximum tolerance is
0.005250000000000821 mm (increase 0.005249000000000821 mm), separately reported
and below the declared 0.01 mm. The 0.02 mm case reports `stitch_gap_exceeded`
and leaves both authored source bodies unchanged.
Reproduce with the accepted `build-preview-release` cache, private Qt runtime and
OCCT 7.9.3, at no more than `-j4`:

```sh
cmake --build build-preview-release --target alloy surface_stitch_test -j4
ctest --test-dir build-preview-release -R '^surface_stitch$' --output-on-failure
python3 tests/surface_stitch_api.py --app build-preview-release/alloy.app/Contents/MacOS/alloy --output ../Alloy-reference-records/validation/surface-stitch-2026-10-20
```

The focused C++ fixture independently checks B-rep validity, shell topology,
edge-to-face adjacency, area and bounds, then retained identity, source edits,
Undo/Redo, reopen, pre-existing tolerance refusal, stale references and partial
seams. The 20 x 10 mm sheet pair uses 0, 0.005 and 0.02 mm gaps at 0.01 mm.
The application fixture exercises shared API/native-dock sessions, preview
isolation, Cancel, commit, retained tolerance edit, Undo/Redo and save/reopen,
with saved sources/results and actual GPU framebuffer captures.

No physical/native actual-hit acceptance, matched visual parity, Windows/Linux
runtime qualification or broad Stitch replacement claim follows from these tests.
