# Center Point Arc Slot candidate (#252, 2026-10-13)

The independent public sketch-lesson contract is recorded outside this
repository in `../Alloy-reference-records/knowledge/tutorials/after-countersunk-plate/independent-contract.md`
and `slot-command-contract.md`. The source picks a path center, two path ends,
and a full width; it later dimensions the construction path R150, one rounded
end R15, aligns the path endpoints horizontally, and dimensions a construction
radial line 135° from the preceding straight-slot guide. This yields the upper
clockwise quarter-arc band; the implementation retains the chosen open sweep
branch rather than hard-coding 90° at creation.

The shared `sketch.centerPointArcSlot` command and native Sketch > Create > Slot
action use the same sketch authority. `arcSlotGroups` retains the construction
path arc and four analytic boundary arc IDs. Path center/endpoints and one cap
radius per group are independent solver values; the side arcs, other cap, and
tangent contacts regenerate from them. Constraints on the path endpoints and
dimensions on the path or either cap remain live. Independent constraints or
dimensions on derived contacts/sides refuse, including on loaded authored data.
The native tool accepts four pointer picks and typed path radius, Tab direction,
signed sweep, and full width with Return; Escape cancels the tool. Its finite
creation domain is `0 < |sweep| < 180°` on the retained branch. Typed values
set initial geometry and do not create driving dimensions: the typed capture
correctly still shows four degrees of freedom. This is a bounded Alloy policy,
not a claimed public initial default. The fourth pointer stage shows a
disposable analytic arc-band preview.

Focused core validation passed the source sequence in one sketch with a
preexisting straight slot: arc DOF 4 → 3 → 2 → 1 → 0 after Horizontal, R150,
R15, and D135. Two closed profiles remain; the arc has analytic area `2475π`
mm², inner/outer radii 135/165 mm and an upper clockwise 90° sweep. Editing
the path to R170 changes its area to `2775π` mm² while retaining authored IDs;
R10 refuses atomically. Undo/redo and save/reopen preserve exact authored data.
An additional second-arc control proves separate cap-radius unknowns and three
closed profiles. The final core test and app build pass after that correction;
the core log is `build-arc-slot/evidence/core-final/center-point-arc-slot.log`
and the rebuilt app SHA-256 is
`f8680979fde5d8b8aabc72965a96ed4ac36421bd2c59985efd44606631df711c`.

The focused native Qt test passed in 5.65 s with one initial harness activation,
no focus repair between user steps, actual typed fields and four pointer picks.
The settled typed, preview and rough two-slot captures were inspected at
`build-arc-slot/evidence/native-10/`. The private editor exited. This fixture
demonstrates native creation and presentation; the source constraint sequence
is separately covered by core and real-app API.

The real-app shared-API run passed 44 calls at
`../Alloy-reference-records/deliverables/center-point-arc-slot-2026-10-13/api-attempt-01`.
It covers both slots, source constraints and dimensions, two selectable
profiles, path-radius edit, exact history, and reopen. Its executable SHA-256
was `8458d91465a22de38c766917e50fdaabb1f09200922c277a08192d8242441088`.
The subsequent multi-group solver correction was checked by the focused core
test and rebuilt app; the API run predates that narrow correction. The later
full sketch lesson, general slots beyond the bounded open-sweep domain, and
constraints on analytically derived boundary contacts remain separate work.

## Independent acceptance

Accepted the bounded #252 candidate `6ea27e0f304b598a5f7e1b668ed8a7c35253d80c`
after source, focused core/native logs, actual GPU captures, API report and
authored-file review. The reviewer did not implement the candidate or launch
an additional editor. Each group maps its two cap UUIDs to its own radius
unknown; the final core control changes a second group's radius without
changing the first. Loaded-data guards reject independent constraints on
derived contacts/arcs, and post-solve regeneration checks the retained boundary.

The inspected saved document exactly equals the API report's source authored
state. Its radial construction line retains the left path-end UUID and origin
UUID; the path is clockwise −90°, with separate straight-slot and arc-band
regions, areas `7500 + 625π` and `2475π` mm², and zero DOF. The R170 edit
retains the region identities and yields `2775π` mm² for the band. Existing
exact history/reopen assertions and atomic R10 refusal provide proportionate
coverage; the final multi-group correction has core/build evidence rather
than a repeated unchanged single-group API run.

The native typed, rough and preview images and API `defined-two-slots.png`
show smooth bands, readable palette text and the source dimensions. The
native fixture uses initial-only harness activation, action activation and
actual Qt pointer/key events; it proves creation controls, not the complete
source constraint gesture sequence or physical OS input. That final sequence
is covered by the shared API. Typed values establish initial geometry without
adding driving dimensions. The finite sub-180° sweep and restrictions on
derived-boundary edits remain explicit; this is not acceptance of the full
31-step lesson or general arc-slot parity.

## Main integration

Integrated as `5beec9c` with review `1e62c33`. Main application rebuild and
three focused arc-slot/straight-slot/fit-spline core tests pass (3/3, 1.55 s).
Astra checked merged tool cleanup and shared solve/evaluator routing. The review
also found that a new straight slot after an existing arc slot could preview
the old arc group. Both callers now explicitly pass the intended slot type;
the renderer never substitutes another group type. Astra source review and the
final application rebuild pass for this presentation-only correction. Prior
native/API evidence is reused with this qualification; no full GUI replay.
