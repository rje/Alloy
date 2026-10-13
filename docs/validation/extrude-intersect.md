# Retained Extrude Intersect — SOL-031 / #219

2026-10-13 candidate from `adf9bbf`. Child of #161, which retains broader operation
conversion/body migration, remaining edit coverage and typed-Return/focus questions.
This slice does not close that parent.

## Command and source contract

External source `kb-extrude-20261012`, rechecked 2026-10-13, documents Intersect,
Profile Plane, One Side and Distance. The documented Objects To Cut policy is
Cut-specific. A single retained same-owner Intersect target, initial sole eligible
target discovery and explicit ambiguity refusal are Alloy scope choices. Original
URLs and operation-menu visual remain in external reference records. Exact public
Intersect discovery, preview styling and target-field defaults are unknown.

The existing `solid.extrude`/`command.begin` authority accepts `operation:
"intersect"`, stable `sketchId` plus exclusive `profileId`/`profileIds`, signed
`distance`/`units`, and exactly one retained `targetBodyIds` entry. Start remains
Profile Plane, Direction One Side and Extent Distance. Default remains New Body;
changing an existing feature's operation still refuses identity migration.
The native operation combo and Target Body field invoke those same commands.
Automatic initial selection only chooses one eligible visible same-owner body;
multiple candidates require explicit choice. Stored targets are never rediscovered
on recompute. Existing IDs, ownership and ordered dependency guards remain active.

The evaluator uses established OCCT Common with the united prism tool group,
non-destructive input handling and existing Modified/Generated support-face/edge
transport. One authored modifier owns no cutter or new body UUID. Target/body and
producer identities survive; `lastModifierFeatureId` identifies Intersect. No new
Join/New Body cap lineage is invented for clipped intersection faces.

The result must be one valid closed positive-volume connected solid. Empty,
touch-only, disconnected and mixed results refuse before document/history
publication. Full containment is valid even with zero removed volume. Evaluated
summary includes `overlapVolumeMm3`; existing removed-volume metadata remains.
Preview displays the real retained overlap in the existing blue Extrude style,
with selected profile IDs and distance cue. For compound profiles the first
profile anchors only the cue; it does not replace or filter authored selections.
The preview and direct commit therefore agree when only part of a compound prism
intersects the target. Cancel restores exact live document/history/presentation.

## Bounded evidence

Affected app and core/native targets build with Qt 6.11.2, OCCT 7.9.3 and
AppleClang 17 on macOS arm64.

- `profile_intersect`: supported top-face circle with signed -6/-8 distance,
  analytic 150π/200π overlaps, upstream diameter 12 yielding 288π, retained IDs,
  one modifier/no cutter, undo/redo and exact save/reopen. Origin-plane full
  containment keeps 4000 mm³ with zero removal; no-overlap, touch and disconnected
  selected regions refuse atomically. Existing Cut/Join focused regressions cover
  the shared gates: all three focused CTests pass (3/3 in 1.48 seconds).
- `tests/profile_intersect_api.py`: real entrypoint, inch quantities equivalent to
  6/8 mm, origin-plane partial overlap 1800/2400 mm³, initial target discovery,
  preview/Cancel equality, retained feature edit, failure/commit refusal,
  undo/redo and exact reopen. Multiple eligible targets do not guess. Two disjoint
  source profiles, one outside the target, preview 20 mm³ correctly while retaining
  both profile IDs. This is the focused independent-review regression.
- `intersect_controls_test`: **3/3 pass in 2.868 seconds**, process exit 0. Fixture
  authoring, source selection and dialog activation use shared API; actual Qt
  distance spin keys, focused combo End selection and pointer Cancel/OK verify
  the new operation and 1800 mm³ preview/result. Native text/Return/global E and
  physical OS input are not claimed.

Actual readable native preview, API preview and final retained body captures were
inspected. Evidence in the isolated `build-intersect/evidence` includes
`core-test.log`, `qt-test.log`, `native-intersect-preview.png`, `api-test.log`, `api/report.json`,
`api/intersect-preview.png`, `api/retained-intersect.png` and
`api/retained-intersect.alloy`. API harness exited 0. No private editor remains open.
Public/Alloy camera, theme and scale are unmatched; workflow, geometry, input and
visual readability are separate evidence, not pixel parity.

Test scaffolding initially attempted global E without opening the dock; that route
remains unqualified here. A teardown stall was the owned fixture's expected
unsaved-design decision, resolved by saving before close. No popup product defect
is inferred. The API session fixture also initially supplied distance to the
selection-only `solid.extrudeDialog` schema; it now uses documented
`command.begin` inputs. Those were test request/cleanup issues, not modeling fixes.

Astra reviewed source, final native log/API report and three captures, including
compound-preview correction, and reported no remaining bounded blocker. Final
integration and issue acceptance remain root-owned. Other platforms and broader
Extrude modes remain unqualified.
