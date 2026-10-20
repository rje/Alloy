# Retained circular sketch-circle Pattern candidate

SKT-177/#700, 2026-10-20. Depends on the accepted rectangular Pattern ownership
and source-only solver foundation. This candidate adds numeric Full and Angle
patterns of one ordinary authored Circle around an authored standalone sketch
Point. Root owns issue, backlog, inventory and continuation-context updates.

The permitted public contract/provenance is external in
`../Alloy-reference-records/knowledge/specs/skt097-sketch-patterns-2026-10-20.md`
(R2/R3, retrieved 2026-10-20), with engineering choices in
`skt097-architecture-2026-10-20.md`. No proprietary application was operated.
Public documentation establishes Sketch > Create > Circular Pattern, Objects,
Center Point, Angular Spacing Full/Angle, Quantity, Total Angle and double-click
editing through a glyph at the pattern center. It does not establish the
untouched defaults, total/source Quantity semantics, rotation sign, exact colors,
layout or glyph art. Those remain Alloy policies or unknown visual parity.

## Shared retained command

Sketch > Create > Circular Pattern opens the existing embedded Pattern command
dock with Objects/Center Point identity selectors, Angular Spacing Full/Angle,
Quantity, and Total Angle. Full displays 360 degrees read-only; switching to Angle
initializes 120 degrees. The Alloy initial policy is Full, Quantity 6. Quantity
counts **total instances including the source**. Positive angles rotate in
positive sketch UV orientation. The original independently authored ring glyph is
anchored exactly at the retained Center Point and double-clicks into the same
editor. Detailed native styling is not an exact-match claim.

The native controls call `sketch.patternPreview` with `type=circular`,
`sourceEntityId`, `centerPointId`, `quantity`, `mode=full|angle`, and
`totalAngleDeg`; edits also pass `groupId`. Existing rectangular inputs remain
compatible. `sketch.patternAccept` and `sketch.patternCancel` share the same
transient state and authored/history authority for both kinds. Preview and Cancel
preserve the accepted sketch/history. OK creates one distinct sketch-history
item; Finish Sketch commits through the document authority. Generic inspection
and Look At preserve/frame the pending pattern. Native editor sessions retain
session identity and refuse stale cross-session acceptance.

Alloy bounds: one Pattern group per sketch, one ordinary Circle, one standalone
Point, integer Quantity 2..16, Full exactly 360 degrees, Angle
0.001 <= Total Angle < 360 degrees, generated coordinates within ±100000 mm.
The seed must be at least 0.001 mm away from the pattern center. Touching or
intersecting member circles refuse atomically. Circular source geometry and
center remain editable; projection/Offset/Mirror results do not become sources.
Opening the other Pattern family in an already-owned sketch returns an
association error rather than silently changing its type.

`parameters.patternGroups` retains an eight-field circular definition: UUID,
type, source Circle UUID, center Point UUID, mode, Quantity, Total Angle and
indexed instances. The original Circle/center keeps its identity at `k=0`.
UUIDv5(group, versioned source/k/role) determines each result Circle/center UUID,
independently of count, mode and angle. Surviving/regrown indices keep identity.
Full uses `theta=2*pi*k/N`; Angle uses `theta=angle*k/(N-1)`, both applied to the
solved source-center vector about the solved pattern Center Point. Radius comes
from the solved source. No source constraints are copied.

The full owned model validates before result rows are removed from PlaneGCS.
Only source/center intent is solved; outputs are derived afterward into the
existing redundant display/profile caches. Results add no free solver DOFs.
Derived diagnostic freedom depends on source Circle and center Point freedom.
Generated drag, constraints/dimensions and retained-source use refuse. Delete of
source, center Point/entity or a generated member refuses with association
feedback. Existing explicit downstream profile sets retain their selected IDs;
count reduction removing a referenced hole refuses before preview or draft
history changes. Count growth does not automatically select extra Cut holes.

## Evidence

Build: `build-preview-release`, Release, private Qt 6.11.2, OCCT 7.9.3, Apple
Clang, `-j4`. No CI, tag or release was run. External evidence:
`../Alloy-reference-records/validation/skt177-2026-10-20/`.

- `sketch_circular_pattern_test`: exact six-instance Full centers, Quantity 8
  surviving IDs, Angle four at 120 then 150 degrees, source identity, source-only
  DOF, invalid center/count/angle/collision and source-at-center atomic refusals,
  owner Delete/derived drag/Fix refusal, source-radius/source-center/pattern-center
  edits, distinct history and exact authored save/reopen. The existing rectangular
  core fixture also passes.
- `tests/sketch_circular_pattern_api.py`: the final real-executable 90-call run
  passes shared preview/Cancel/OK, generic inspection preservation, Full6→8 and
  Angle4/120→150 exact trigonometric checks, stable profile IDs, source edits
  through an existing six-profile Cut, missing-profile refusal, document Undo/Redo,
  exact authored reopen and STL export. The fixture starts the center at (25,25),
  seed at (37,25), R1.5, then changes the center to (26,24), seed to (38,26) and
  radius to 2. The 55 × 50 × 5 plate has exact final volume
  `13750 - 6*pi*2^2*5 = 13373.008881569229 mm³`.
- Independent stdlib STL readback checks aligned finite unit normals, paired
  oriented manifold edges, one positive-volume component and Euler −10. It
  independently assigns 864 wall-vertex samples to each of the six analytically
  predicted centers, radius 2 and full thickness 0..5. The mesh has 3492 triangles;
  mesh volume is 13373.12848875577 mm³ and is separately reported in `report.json`.
- `sketch_circular_pattern_controls_test`: action-triggered Create opening, then actual-hit Cancel/OK,
  center glyph double-click, Quantity 6→8, Angular Spacing native popup selection,
  four-instance 120-degree Angle, typed 150-degree edit, Cancel, commit and distinct
  Undo/Redo. The fixture observes popup readiness/closure and actual focus before
  keys; it performs only initial harness activation and no inter-step focus or
  selection repair. Final native run: 3/3 Qt functions pass in 5.470 seconds, including
  exact glyph-center pixel alignment and a disabled Full-mode Total Angle field.
- Existing rectangular actual-hit shared-controls regression: 3/3 Qt functions
  pass in the final 4.143-second run. This is separate from the circular native workflow.

Actual Full/Angle controls, center glyph, analytic-circle profiles and six-hole
solid GPU captures were inspected. Apple M4 / Apple, OpenGL `4.1 Metal - 90.5`,
DPR2 and native FBO dimensions are recorded. No new renderer/backend policy was
introduced. These are Qt-delivered component inputs and local macOS GPU evidence;
physical OS input, native DPR1 and packaged Windows/Linux runtime remain release
gates. Independent Astra source/evidence review accepted the bounded candidate,
recomputing Full/Angle/final centers to 1.8e-15 mm maximum error, 32 UUIDv5
fields and independent STL topology/volume. This review found no blocker.

## Explicit remainder

SKT-182/#705 (Todo, owned by root) preserves circular canvas source/center picking
and angular/quantity manipulators. The numeric selector route does not establish
those interactions. Symmetric, suppression, arbitrary/multiple seeds and copied
constraint semantics remain ADV-005. Public screenshot styling, untouched
defaults, center-pick eligibility beyond this standalone Point and glyph art
parity are unknown. No broad exact-match claim is made.


Final candidate paths: `api-candidate/`, `native-candidate/`,
`native-candidate.log`, `core-candidate.log` and
`rectangular-core-candidate.log`. The final shared rectangular controls regression
is `rectangular-candidate.log`; its captures are separate. API and native fixtures
both close their private editor. No user-owned instance was altered. The
integrating agent owns independent review and issue closure.
