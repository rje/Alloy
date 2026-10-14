# Overall Slot candidate — SKT-069

Candidate on `feat/overall-slot`, 2026-10-13. The public command sequence is
recorded outside the repository in
`../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-13/overall-slot-contract.md`.
Independent compatibility review is pending.

Create > Slot > Overall Slot accepts the first outer endpoint, total end-to-end
length, Tab direction angle, second outer endpoint and full width. The tool
remains active for another slot; Enter or Escape leaves it. The shared API
`sketch.overallSlot` takes two outer endpoints and full width. It creates two
analytic straight sides, two semicircular arcs and a construction centerline.
Three retained driving dimensions own total length, width and angle; the
center spacing is `length - width`. Existing Center Point Slot is unchanged.
Edits rebuild the same group and profile IDs. Alloy refuses `length <= width`
atomically rather than guessing a degenerate circle. The exact reference
defaults and glyph placement are not established by the public contract.
Alloy currently preserves the group center as the dimension-edit gauge, so
changing width while retaining total length can move the initially clicked
outer extreme.

Focused evidence:

- Core `overall_slot` and existing `center_point_slot` pass 2/2. The new test
  verifies length 30, width 6.5, analytic area and bounds, independent
  length/width/angle edits, invalid edit atomicity, history and exact reopen.
- Qt native `overall_slot_controls` passed 1/1: first endpoint click, typed
  total length 30, Tab and typed 20-degree angle, second endpoint click, typed
  width 6.5, side click and Enter to leave the repeatable tool. Input went
  through the actual sketch controls without focus repair. The native capture
  was inspected.
- A 29-call real-app API run passed Box stock, one Overall Slot profile,
  through Cut, length/width/angle edits, invalid edit refusal, stable group
  identity, undo/redo and exact save/reopen. The cut removed
  `5 * (23.5 * 6.5 + pi * 3.25^2)` cubic millimeters. The resulting viewport
  capture was inspected.

Evidence is external under
`../Alloy-reference-records/deliverables/overall-slot-2026-10-13/native-02/`
and `../Alloy-reference-records/deliverables/overall-slot-2026-10-13/api-01/`.
The native capture precedes one presentation-only adjustment: the new angle
label now starts 10 mm on the negative perpendicular side of the first arc
center to clear the profile. Core tests and app/native targets rebuilt after
that adjustment; a refreshed capture remains to be inspected. Run only the
focused native test, with `ALLOY_OVERALL_SLOT_EVIDENCE` set to a new external
attempt folder, when the sole editor lease is available. The old capture and
first timed-out native harness attempt are retained as separate evidence.
