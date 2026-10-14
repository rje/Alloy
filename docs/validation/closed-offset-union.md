# Explicit closed Offset union Cut

Candidate `705591b` is independently accepted for bounded core/API authority and
shared-command visual preview. Integrated at `8241b66`; main app build and focused
Offset core pass. SKT-074/#330 stays open while the dimension-test failure below
is diagnosed. This is an Alloy opt-in policy, not established
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

Native combo input is unverified: two popup click attempts and a subsequent
keyboard focus probe failed to establish selection. Programmatic `command.update`
is not native row selection. SKT-078/#377 preserves native select/replace/clear,
readability and commit/Cancel qualification, using the accepted TUT-021 fixture.
TUT-021 remains accepted within its original ordinary-profile/API scope.

The existing `extrusion_document` test reports `Driving dimension does not exist`;
its target builds, but this run does not establish whether that failure predates
the candidate. Ordinary stale-cell refusal evidence remains in
[closed rectangle Offset](closed-rectangle-offset.md), without a repeated run.
