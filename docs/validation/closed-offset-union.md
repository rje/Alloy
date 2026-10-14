# Explicit closed Offset union Cut

Candidate `705591b` is independently accepted for bounded core/API authority and
shared-command visual preview. Integrated at `8241b66`; main app build and focused
Offset core pass. Follow-up `33ea70a` fixes a dangling JSON reference in the older
extrusion test and labels programmatic test setup with API provenance. This is an Alloy opt-in policy, not established
reference aggregate UI behavior.

The authored Cut retains sorted Offset group IDs, exclusively from ordinary
profile IDs. Evaluation reconstructs current solved loops at the feature prefix,
fuses their union, and requires complete current-cell coverage and equal union
area before reusing existing extrusion lineage. Missing groups, partial coverage,
invalid/disconnected unions and mixed aggregate/ordinary selection refuse.
Resolved cell IDs remain transient presentation state. Ordinary cell clicks
replace the aggregate; unavailable sources refuse before accessing a profile.

External evidence is `deliverables/sketch-offset-union-2026-10-14/`:

- Focused `closed_rect_offset_test` passes the three-port union calculation.
- API02 records 19 calls, 17 successes and two intended atomic refusals. Clearance
  1 to 1.1 mm changes three cell IDs, union area 973.5 to 979.43 mm² and Case volume
  59639.83399055 to 59619.82398438 mm³. Undo/redo and exact authored reopen pass.
- Qt visual04 reports 3/3 for shared-command aggregate input, valid preview and
  Cancel. Independently inspected capture shows blue resolved-region highlights.
  The shorter `Offset union · 3 loops` label has source/build evidence only.

SKT-078/#377 adds a native Qt-delivered keyboard route through the open Profiles
combo on the accepted TUT-021 fixture. Native02 passes aggregate selection and
preview, ordinary-profile replacement, direct aggregate-to-Clear selection
(invalid preview and disabled OK), Cancel with unchanged authored data, then
aggregate commit and exact undo/redo. The capture shows a readable combo label
and blue resolved-region highlight. The first popup-row pointer-click attempt
did not change selection, so pointer-row selection remains unverified. Native03
repeats the 3/3 pass with keyboard navigation, one initial application
activation, and an assertion that the combo or its popup owns focus before
each key sequence; there is no focus repair between demonstrated steps. The
Clear row now invokes the shared
selection update instead of leaving the aggregate active.
TUT-021 remains accepted within its original ordinary-profile/API scope.

The older `extrusion_document` test retained a JSON reference from a temporary
object in unchanged parent source. An owned dimension ID fixes its Release failure;
the corrected test passes in candidate Release and integrated main builds. Ordinary stale-cell refusal evidence remains in
[closed rectangle Offset](closed-rectangle-offset.md), without a repeated run.
