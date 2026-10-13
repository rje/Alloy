# Exact-offset array Join investigation

SOL-039/#265, 2026-10-13. Historical diagnostic findings below are preserved.
Accepted #261/#263/#266 now permit actual retained Intersect and eight-body
Pattern. The current bounded correction addresses coverage integration;
cumulative application acceptance remains separate.

## Exact fixture and authority

External `validation/ornament-feasibility/authoritative-profile.cpp` builds the
80 mm / 5 mm / R50 revolved source and inward Shell 5 mm. Candidate #261 actual
SketchRegions evaluates an open fitted spline through learner-chosen points
(-4,-1), (6,19), (-5,45), (3,89), retained signed Offset -5 mm and two explicit
Line connectors. GeometryKernel extrusion and Intersect produce one valid
positive-volume member. Eight rigid rotations at 45° increments and the source
Ø10 × 5 mm hub at z=-5..0 fail their cumulative Join. These fit points are
fixture choices, not source-prescribed dimensions. Source alias:
ornament-pattern-20261013; public evidence remains external.

Diagnostic linkage uses the built candidate #261 kernel/solver and its current
CadDocument object, with Qt 6.11.2 and OCCT 7.9.3 on macOS arm64. This extends
the authoritative profile fixture; the shell and array remain external kernel
construction rather than retained application commands.

## Material findings

- The exact profile face, prism, Intersect member, hub and first-member/hub Join
  pass BRepCheck and OCCT Boolean argument checks including self-interference.
- The profile/prism maximum edge tolerance is 5.19e-7 mm. The Intersect member
  reaches 0.00426379 mm under OCCT's existing exact-surface processing.
- Two members plus the hub fail with BuilderFailed, BuildingPCurveFailed,
  AcquiredSelfIntersection and TooSmallEdge. Eight members return an invalid
  fifteen-solid result; multiple faces report BadOrientationOfSubshape.
  Simplification does not cure this: the unsimplified output is already invalid.
- Hypothesis 1: rigid transforms sharing geometry (`copy=false`) rather than
  deep copies reproduce the same two-member failure. Pattern transform policy
  remains unchanged.
- Hypothesis 2: forced OCCT SameParameter through immutable BRepTools_ReShape
  reconstruction, at 1e-7 mm kernel precision, lowers the worst seed tolerance to
  0.00193433 mm but does not cure two-member Join. Exact 3D curves are retained.
- OCCT's documented gluing modes require no real subshape intersections. This
  fixture has them, so a gluing shortcut was not tested.

Hypothesis 3 also fails: OCCT GeomConvert_ApproxCurve replaces only the evaluated
offset boundary at a requested 1e-6 mm tolerance. Reported MaxError is
9.72417e-7 mm; 1,001 parameter samples reach 9.57125e-7 mm point error and
independent nearest-source normal-distance error 6.80858e-7 mm. The replacement
planar face is valid. Actual GeometryKernel extrusion initially refuses its
nonadaptive area/volume integration mismatch; that refusal is preserved. An
isolated equivalent prism with adaptive area and Gauss–Kronrod volume agrees with
area × 60 mm within the existing relative 1e-9 bound. Kernel Intersect returns
one valid member (~2,274.07 mm³), but the same eight-member Join still yields an
invalid fifteen-solid result (~16,672.4 mm³). No production fallback is warranted.

The evidence establishes a Boolean intersection representation problem on valid
inputs, but does not isolate its precise face/curve pair. Replacing the exact
offset boundary alone is not a demonstrated cure. Rotated shell-derived support
faces and their repeated coincidence/intersections remain a structural suspect,
not a confirmed cause. No tolerance search, relaxed validity gate, dimension
change, simplified source solid or discarded result is used.

## Retained reproduction plan and remaining decision

The first actual-app source prefix now exposes an earlier failure. Root's
`tests/ornament_source_api.py` authored the constrained profile, Full Revolve,
Shell, retained hatched section and exact offset strip from blank. Shell volume
is 37826.7374468 mm³ and the one closed XZ strip has area 510.8141335884 mm².
The 60 mm retained Intersect refuses with `boolean_empty_result`, unlike the
headless probe. This is not a successful seed or full tutorial. External
`deliverables/ornament-2026-10-13/source-prefix-01/` preserves the 46-call
transcript, actual pre-Intersect inspected authoring and captures; its separate
authored JSON was extracted from that inspection, not saved by the application.
The editor exited. Localize the discrepancy before claiming the raw kernel
probe predicts the retained workflow. Tiny numerical or topology differences
are hypotheses, not an established cause. The runner now saves a pre-Intersect
document on its next invocation; that added save has not yet executed.

After #261/#263 are accepted, reproduce from a fresh design through the shared
application commands. Reuse the accepted #260 source fixture to author the
constrained XZ radial profile, Full Revolve and retained generated bottom-face
Shell 5 mm. Create the disclosed four-point XZ spline, retained signed Offset
-5 mm and two explicitly snapped Line connectors, then Intersect one 60 mm prism
with the shell through the authoritative feature evaluator. Save the pre-array
state and exact geometry summaries.

Create the retained full Circular Body Pattern on origin Z with total quantity
8. Save the eight-body design and collect stable IDs. On XY author the Ø10 hub
and extrude -5 mm with all eight IDs as Join participants. Record either its
structured refusal and exact untouched candidate/undo state, or one valid
connected output containing every complete participant and hub. This exact
retained reproduction is the first acceptance gate for any correction.

A possible next structural experiment would partition the original shell once
with all rotated cutting strips, avoiding repeated Boolean intersections among
independently trimmed coincident shell faces. Its geometric equivalence and
preservation of the source Intersect → Body Pattern → hub Join sequence are
unproven. Independent Astra diagnosis precedes any further experiment. No
approximation fallback or tolerance policy is proposed after the failed variants.

If a correction succeeds, verify meaningful upstream source edits, stable result
ownership, exact undo/redo/reopen, and one strict closed STL. Follow with the
transverse Ø4 hub bore and independently verify both exits. API/native/GPU/source
comparison evidence remains separate; the full tutorial gate is still required.

External evidence directories: `topology-diagnostic/`, `shared-transform/` and
`same-parameter/` and `approx-offset/`, with source/build helper alongside them. These are headless
geometry diagnostics; no editor was opened. Actual retained Pattern/Join, source
edit, history, reopen, strict closed STL and transverse bore checks remain required
before #265 or the full ornament can be accepted.

Library evidence (retrieved 2026-10-13):
[OCCT 7.9.3 gluing contract](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/BOPAlgo/BOPAlgo_GlueEnum.hxx),
[SameParameter declarations](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/BRepLib/BRepLib.hxx).

## Retained prefix and stopped shared partition

The actual application source prefix at main 22d8237 independently reproduces a
prior refusal: the same constrained source, Shell, Section, exact retained strip
and explicit Lines reach one profile, but the 60 mm Intersect refuses
`boolean_empty_result`. External `deliverables/ornament-2026-10-13/source-prefix-01`
retains the complete command transcript, pre-Intersect authored JSON and inspection.
This earlier refusal must be resolved before retained Pattern/Join acceptance.

Astra proposed one shared ancestor CellsBuilder partition: unchanged shell S,
exact prism T0, its 45° rotation T1 and hub H; select
`H OR (S AND T0) OR (S AND T1)` with common material 1 and remove boundaries once.
The prerequisite checks now stop that experiment before partitioning. No all-parts,
selected result, material-equivalence classifier run or eight-member expansion is
claimed.

The first retained diagnostic mistakenly used an eager Common constructor before
setting non-destructive flags. Its post-operation metrics were confounded. The
external `retained-prefix/method-confound.txt` labels this attempt. The corrected
`retained-prefix-clean` captures untouched shapes and verifies serialized hashes
before/after OCCT argument checks. Independent `raw-inputs-clean` checks the
original raw shell/prisms before any Common or partition.

Clean retained and raw checks both establish an original Shell issue:
BRepCheck-valid, closed, one solid, but Boolean self-interference status 2. Their
worst edge tolerance is approximately 0.8038713 mm. The reported fault contains
inner face 3 (`Geom_OffsetSurface`), its seam and adjoining edges/vertices; it does
not identify the outer revolution face. Inner seam edge 5 has a measured
curve/pcurve deviation of 0.765708011 mm. Its end vertices are approximately
(-2.73098, 0, 0) and (-0.071672, 0, 73.93430) mm, with the same high tolerance.
This spans the inner seam rather than establishing an apex-only defect. The
precise self-intersection parameter pair remains unknown. The prior regular-wall,
opening and BRep checks remain their recorded bounded evidence; they did not
establish Boolean-argument validity for downstream operations.

A distinct rotation defect affects the diagnostic prism: the original exact
prism passes both checks, while a deep-copy 45° transform fails Boolean
InvalidCurveOnSurface and cap-face UnorientableShape. The offset boundary's cap
pcurve deviation reaches 5.28355e-7 mm against a 5.18806e-7 mm edge tolerance.
The same rigid rotation with shared geometry/location passes BRep and Boolean
argument checks and preserves the source hash. This is a narrowly demonstrated
rigid-rotation representation remedy, not a cure for Shell or array Join. #263
owns its actual retained Pattern, lineage and immutability qualification; Mirror
reflection policy remains separate.

Evidence: `retained-prefix-clean/{report.json,shell-subshapes.json,prism1-subshapes.json}`,
`raw-inputs-clean/report.json`, `shared-prism-rotation/report.json` and their input/
fault BReps. The read-only inspection and build helpers are external alongside
these records. No GUI, source simplification, tolerance alteration, production
approximation or CellsBuilder patch was introduced. Shell robustness, rigid
rotation and existing Join-lineage non-destructive processing require separate
bounded ownership and review.


## Retained coverage correction candidate

Actual cumulative-01 reaches accepted exact Shell, strip Intersect and Pattern8,
then refuses hub Join with `join_lineage_ambiguous`. The saved eight-body design
reopens through CadDocument. A configured non-destructive Fuse returns one
BRep/Boolean-valid closed solid; participant-minus-result Cut contains no residual
solids for all eight ribs and hub. This establishes a different retained failure
from the invalid raw fifteen-solid result discussed above.

Coverage previously compared topology-dependent `summarize(input)` adaptive
integration with default fixed integration of Common. Each rib Common underreports
by about0.005 mm3. Even topology-dependent adaptive summaries differ enough to
fail the unchanged material threshold. Explicit Gauss–Kronrod on both shapes,
requested1e-11/OnlyClosed=true/IsUseSpan=false, reports maximum reached relative
error1.5581e-10 and maximum pair difference1.3527e-5 mm3. Adding both estimated
absolute errors remains below the existing2.2781e-5 mm3 threshold for every rib.
No material threshold or kernel tolerance is relaxed.

`covers()` now uses that consistent integration independently of offset-edge
representation. It checks Common validity, handles no-solid overlap before
integration, refuses nonfinite/failed/unconverged estimates, and includes both
reported absolute errors in the unchanged coverage comparison. There is no
geometry approximation, altered Fuse, dimensions change or residual fallback.

The focused profile_join, successive_join and join_lineage_immutable regressions
pass3/3. The retained external source-specific Join now returns successfully. The probe
then stops at original participant byte preservation: additional located surface
and pcurve representations were added to a shared edge. Existing 3D curves match;
independent raw-diff reconstruction classifies every change as additive support
representations. Original geometry, parameter ranges, tolerances and topology
are exact under index remapping; serialized bytes are not identical.
The saved candidate joined BRep now passes all nine configured input-minus-result
Cuts: successful non-destructive Boolean, valid residual, and no remaining faces,
edges or vertices. The independent raw-diff review classifies the additive support
representations. Real application cumulative-02 also passes hub Join; its downstream
bore/edit/history/reopen/strict-STL acceptance remains separate. No permanent repository fixture reproduces this
source-specific numerical trigger; the saved retained external fixture and probe
are required to replay it. Existing exact-offset Pattern core fixture is an
untrimmed prism, not the same shell-derived rib, so it was not broadened into a
different geometry claim.

External evidence: `validation/ornament-feasibility/retained-join-coverage.json`,
`retained-join-coverage-tight.json`, `retained-join-candidate.cpp`,
`retained-joined-candidate.brep`, `retained-joined-containment.cpp`,
`retained-joined-containment.json`, `join-input-additive-diff-review.json`,
`join-coverage-core-final.log`, and the saved
`deliverables/ornament-2026-10-13/cumulative-01/ornament-pattern-eight.alloy`.
The raw report's no-residual-solids observation is qualified; the final candidate
probe additionally rejects residual faces/edges/vertices. Independent Astra review
and real application cumulative source/edit/through-bore/strict-STL acceptance
remain required before closing #265/#264.


The first completion probe stopped at byte-preservation (additive support cache)
and then used BRepAlgoAPI_Check on an empty residual, which is not a valid Boolean
argument. Its incomplete report is not a pass. The final containment probe reuses
the saved joined BRep and applies BRepCheck_Analyzer to residual validity plus
explicit no-face/no-edge/no-vertex checks. Maximum conservative pair discrepancy
including both reported integration errors is1.4163313e-5 mm3. The tighter GK
run reached maximum relative estimate1.55805024e-10; these are library estimates,
not mathematical error bounds. No GUI was launched in these headless checks.
## Independent acceptance of the numerical correction

Astra reviewed `edcd76e` plus `48125ca` (integrated as `a22c676`/`a3f6f4e`).
Accepted for the bounded Join coverage calculation: both inputs use the same
explicit adaptive integration, and the sum of estimated absolute errors consumes
part of the existing material threshold. No kernel tolerance, geometry, Fuse
policy or material-loss threshold is relaxed. The achieved errors are estimates,
not rigorous mathematical bounds.

Independent source/evidence review confirms the focused regressions pass 3/3
in 2.03 s. The saved actual corrected Join result preserves all eight ribs and
hub: each configured input-minus-result Cut succeeds, is BRep-valid and contains
no face, edge or vertex (`retained-joined-containment.json`). The independent
raw-diff classification establishes preservation of existing geometry, tolerances,
parameter ranges and topology despite added OCCT support representations; byte
identity is explicitly not claimed.

The actual cumulative-02 Join now succeeds. Its inspected
`eight-ribs-hub-join.png` shows the complete selected rib assembly and bottom hub,
one visible retained body and no dialog obstruction. This is actual API/GPU
Join evidence, not independent geometric proof from an image or native input
parity. The external retained source fixture reproduces the numerical failure;
the ordinary regression fixtures do not claim that exact trigger.

This acceptance covers the numerical correction and retained Join only.
Downstream bore, source edit/history/reopen, strict STL and complete tutorial
acceptance remain separate. No GUI or duplicate model replay was performed by
this reviewer.


## Single-result ownership performance candidate

Actual cumulative-02 hub Join passes, but adding the bore construction plane
recomputes the ordered model and times out at180 seconds. A native CPU sample
places that work in per-input coverage Common Build; the capture recording also
fails after the long frame gap. This is not a complete source movie/tutorial pass.

After independent design agreement, the bounded branch skips redundant Common
only when the completed configured Fuse has exactly one output solid, after the
existing no-mixed-topology and valid/closed/positive-result guards. FUSE is the
union of the explicit inputs, so that result is their unique ownership destination.
Boundary history alone is not used as a proof of material containment. Multiple
outputs retain numerical coverage grouping; actual Fuse face/edge history,
participant order/primary identity, prior-union volume, added-material requirement,
use counts and stored result ledger remain unchanged. No document cache or
geometry strategy changes.

The existing single/multiple-output regressions pass3/3 in1.95 seconds. The actual
retained eight-body fixture plus hub evaluates joinResults in21.774 seconds with
all eight participants assigned to the original first body and result volume
17818.829392961095 mm3. This headless timing differs from the earlier70-second
application command, so it is not a matched speed ratio. The output is saved for
the same independent topology-empty input-minus-result oracle, which passes all
nine inputs with successful valid residuals and no faces/edges/vertices. Cumulative actual
application latency and downstream source gate remain separate.

External evidence: `single-join-core-final.log`, `retained-single-join.cpp`,
`retained-single-join.json`, `retained-single-join.brep`,
`retained-single-join-containment.json`. Library contract retrieved2026-10-13:
[OCCT Boolean operations](https://dev.opencascade.org/doc/overview/html/specification__boolean_operations.html).

## Independent single-result ownership review

Astra accepts candidate `518de75` for the bounded ownership optimization. Source
review confirms only the participant/profile coverage calls are skipped when
successful Fuse has exactly one validated closed positive-volume solid and no
mixed output topology. Under the library union contract every explicit input
has that unique destination. Multi-result coverage, target order, primary UUID,
prior-union/no-addition checks, accepted ownership ledger and actual kernel
history transfer remain unchanged. Boundary history alone is not used as proof
of material containment.

Reviewed final focused core 3/3 (1.95 s), retained actual Join (21.774 s, volume
17818.829392961095 mm3, all eight participants and first-body ownership), and
`retained-single-join-containment.json`: all nine original ribs/hub inputs minus
the new result yield successful valid Cuts with no remaining faces, edges or
vertices. This establishes the retained result after the shortcut, separately
from the earlier coverage correction. No geometry or tolerance is changed.

This is bounded source/core acceptance, not a new GUI or full cumulative pass.
The final bore, edit/history/reopen, mesh and recording remain separate gates.
No duplicate test or model replay was needed for independent review.

## Independent #265 cumulative acceptance

Astra checked the current SOL-039/#265 issue scope against terminal
cumulative-04. Accepted for closure: the actual retained source/profile/Intersect,
Pattern8 and prescribed diameter10/height5 hub Join produce one valid connected
body with stable primary ownership. The original and offset-edited saved models
pass independent BRep/Boolean validity, all eight complete members plus hub
preservation (all nine residual volumes zero), through-bore void samples at both
exits and adjacent retained hub material. Count history, meaningful upstream
offset edit, exact authored undo/redo/reopen and both strict closed oriented
one-component millimeter STL readbacks pass through the actual application.

The failure diagnosis and bounded fixes are independently reviewed above:
invalid source Shell and deep-copy inputs were resolved in their prerequisites;
retained Join coverage used inconsistent volume integration, then redundant
single-output coverage work caused latency. No tolerance relaxation, replacement
solid or hidden invalid result establishes this acceptance. Added OCCT support
representations are qualified separately from original geometry preservation.

Evidence is external cumulative-04 `report.json`, `transcript.json`,
`geometry-verifier.log`, original/edited authored files and STL files, alongside
the preserved kernel diagnostics and prior independent receipts. The issue
explicitly permits disclosed free spline points. Its retained geometry gate is
complete despite the known open-crown visual difference in that fixture; #264's
full tutorial visual gate remains blocked until the corrected fixture is
independently reviewed. This acceptance does not claim full source visual parity
or transfer cumulative-04 results to the changed cumulative-05 geometry.
