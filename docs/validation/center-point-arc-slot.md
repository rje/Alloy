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
