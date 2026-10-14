# Retained Extrude Cut All candidate

SOL-047/#287 adds one-sided **All** extent to the existing Extrude Cut command for one explicitly selected retained target body. The authored feature stores `extent: all`, `flip`, and the source sketch/profile and target body IDs. Its evaluated distance follows the target's current conservative OCCT bounds, including curved targets, and is not saved as a driving length. The native Extent Type menu exposes All and a Flip checkbox; the same fields work through `solid.extrude`, `solid.extrudeDialog`, `command.preview`, and feature edit.

The [external contract](../../../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-13/through-all-cut-contract.md) records the public source, retrieval date and the deliberate single-target limitation. All over multiple visible bodies, non-Cut operations and full shoehorn integration remain outside this child. This is a candidate pending independent compatibility review.

Focused verification on macOS arm64, Qt 6.11.2 and OCCT 7.9.3:

- `extrude_through_all` passed: a circular Cut stays through a box when height changes from 10 to 20 mm; a cylindrical target reaches through curved bounds; Flip cuts an opposite-side box; wrong-side Flip fails atomically; undo and reopen retain All.
- [Real-app API evidence](../../../Alloy-reference-records/deliverables/extrude-through-all-2026-10-14/api-01/) passed 29 calls. It covers preview/Cancel, committed Cut, target resize, edit dialog, undo/redo, save/reopen and GPU screenshot. The original target is unchanged after Cancel.
- [Native Qt evidence](../../../Alloy-reference-records/deliverables/extrude-through-all-2026-10-14/native-06/) passed. A delivered Extent Type key chooses All; an indicator click flips direction, displays the full target-behind-direction error and disables OK; Cancel preserves the authored document. Captures were inspected after layout settled. The native check does not claim an OS mouse gesture.

The fixture is a focused ingredient, not the source tangent-plane shoehorn model. It establishes the retained All option and its edit behavior before that cumulative workflow.
