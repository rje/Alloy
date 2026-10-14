# Surface Thicken solid orientation

Independently accepted and integrated on 2026-10-14. Source review and the
focused core check passed; unchanged presentation did not require another
full-app run. The retained test log is external at
`../Alloy-reference-records/deliverables/shoehorn-workflow-2026-10-14/fillet-orientation-core-01/ctest.log`.

The retained guided-sheet Thicken now places material orientation on its one
shell inside a forward solid wrapper when OCCT returns negative signed volume.
Its six faces and twelve edges remain the same current shapes with stable
source-derived IDs. The positive and negative one-side edits stay valid and
editable; a small downstream corner Fillet now receives a compatible solid.

The focused `surface_thicken` core check compares the previous and new
positive-material representations with zero-volume two-way Boolean
difference. It also checks the forward wrapper, valid downstream R1
material-removing Fillet, signed-side edit, retained IDs, undo/redo,
save/reopen, source-path edit and strict STL export.

This is a kernel orientation correction, not acceptance of the surface lesson's
exact rounds. The actual 16 mm-bend shoehorn fixture still fails the wide-end
R12 result under GEO-019/#352. GEO-018/#350 remains responsible for generated
Fillet face and endpoint lineage plus the later R1.5 perimeter round. The
external `shoehorn-fillet-radius-diagnostic.md` records the exact input,
selected IDs, OCCT statuses and source-geometry uncertainty.
