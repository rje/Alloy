# SOL-032 Sweep foundation candidate

2026-10-13, isolated `sweep-foundation`, base c3861aa. Build uses privateQt6.11.2,
OCCT7.9.3 and AppleClang17. Kernel/API foundation only; #221/#222 source sketch
authoring, #224 native Sweep controls and #225 cumulative tutorial remain
separate. No complete tutorial or matched visual parity claim is made.

The independently authored analytic fixture uses four straight runs and six
quarter-circle arcs, circle section radius1. It is source-shaped geometry, not
proof of authoring the source fillets through the UI. Expected length is
135+16.5π =186.8362787842 mm, volume135π+16.5π² =586.9634808526 mm³ and bounds
[-1,-21,-1]..[15,41,1] mm. The core checks exact section/volume/bounds, unique
builder-provided caps with classifier-proven outward frames, stable feature/body/cap IDs, supported radius/terminal
edits, undo/redo and exact storage. Positive and negative normal alignment pass.

Focused core3/3 passes (Sweep, region evaluator and Revolve regression).
Material refusal controls include center misalignment, missing profile/entity, wrong path order,
non-G1 kink, approximately 1e-8 mm (10 pm) undeclared gap, explicit Coincident correction and branched
selection. Radius2 makes neighboring3mm centerline legs collide; the actual
OCCT geometric self-interference check refuses it as `sweep_self_intersection`,
with accepted authored state unchanged. A separate local-component fixture
checks explicit retained ownership; the shared active-owner helper is reused.

The real app API passed stable reservations, actual preview, Cancel with exact
history, commit, API-only edit session, undo/redo, atomic missing-entity refusal,
radius1→0.8 and terminal extension2mm rebuild, retained cap IDs and exact reopen.
Direct creation/edit leaves no spurious command session. API ownership check
covers the root component. Existing active-owner authority is reused, not
reimplemented. Source profile/path sketches auto-hide only on new commit.

Artifacts under `build-sweep/evidence/api`: report/transcript/app.log,
`sweep-foundation.alloy`, preview and committed PNGs. GPU reports AppleM4,
OpenGL4.1 Metal, DPR2. Actual capture inspection shows a smooth readable gray
pipe, distinct open ends, readable Browser/Timeline and no model clipping. The
preview is geometry evidence with no native control panel. The committed image
shows the edited0.8mm section/extended end, not the initial radius1 oracle.
Editors closed and the private GUI slot was released. Initial runner refusal
assertion used a revision saved before undo/redo; it was corrected to snapshot
immediately before rejection, and the final complete run passes.

Reproduce (serialize app runs with the private editor slot):

```sh
cmake --build build-sweep --target alloy solid_sweep_test sketch_regions_test solid_revolve_test
ctest --test-dir build-sweep -R '^(solid_sweep|sketch_regions|solid_revolve)$' --output-on-failure
python3 tests/solid_sweep_api.py --app build-sweep/alloy.app/Contents/MacOS/alloy --output build-sweep/evidence/api
```

## Independent acceptance

Astra reviewed candidate `580947fb0f0e8c0e887130a08dbb41df26af9484` on
2026-10-13 without changing its implementation or repeating the app run.
The kernel, shared session/dependency paths, focused test source and persisted
core/API evidence support acceptance of #223's circle-section, full-length,
Perpendicular, New Body foundation.

Source review confirms exact solved source-entity edges, retained ordered IDs
and start point, authored Coincident/shared-point connectivity, oriented G1
checks, and no proximity-based path repair. OCCT pipe construction is followed
by solid/volume validation and a geometric self-interference check; the focused
colliding-leg fixture specifically reaches that rejection. Only unique actual
First/Last planar caps receive semantic identities and outward frames. Curved
side naming and cap boundary projection references are not claimed.

The persisted core log records all three focused tests passing in 1.59 seconds.
The real-app report and runner cover preview/Cancel, retained commit/edit IDs,
history, atomic missing-reference refusal, upstream section/end edits, and
reopen. Independent inspection found the saved authored JSON exactly matches
the report's edited document. The edited analytic volume
`0.64*pi*(137+16.5*pi) = 379.6778663423 mm³` matches the reported geometry.
Both actual captures show the entire smooth gray pipe and distinct ends, with
readable Browser/Timeline. The model is relatively small in the viewport and
arc/line seams remain visible; these images establish bounded presentation,
not matched reference styling.

The endpoint-gap fixture is approximately `1e-8 mm` (10 pm), not 10 nm; the
wording above and in the contract is corrected without changing the test or
its meaning. This review reuses persisted runs rather than claiming a new run.
Native controls/input (#224), typed path and paired-fillet authoring (#222/#221),
and cumulative source tutorial (#225) remain separate. The analytic fixture
is not evidence that those authoring steps are available. Root owns integration
and issue closure; the broader Sweep family remains unqualified. See the
[shared contract](../research/solid-sweep.md).
