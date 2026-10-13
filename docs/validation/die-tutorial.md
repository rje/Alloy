# Die tutorial workflow

The complete workflow is implemented in `tests/die_tutorial_api.py`. The final shared-API run passes geometry, component ownership, upstream edit,
history, reopen and strict STL checks. Independent tutorial review accepted the bounded gate; see
[the independent receipt](die-independent.md).
The first attempt failed strict STL readback; it remains archived as diagnostic
evidence separately from the corrected final recording.

The public source is the external TUT-002 ledger
`knowledge/tutorials/warwick/library-die-ledger.md`, retrieved 2026-10-13.
It asks for a named die component, a centered square, cube extrusion, trials of
Chamfer and Fillet, spherical cuts on all six faces through Repeat Sphere, then
component-scoped STL export. Dimensions, pip counts and positions are learner
choices. Our fixture uses a 20 mm cube, previews/cancels both 1 mm edge treatments,
accepts Fillet, and places diameter-3 hemispheres in counts 1 through 6. Opposite
counts sum to seven; the complete face-local placement table is in the run receipt.

The runner checks one retained closed valid solid, six retained planar supports,
all 24 features owned by the die component, and removed volume of 47.25π mm³
relative to the rounded cube. The final gate also checks upstream rounding edit,
undo/redo, exact authored reopen, and an independently read closed millimeter STL.
These history checks extend the source lesson.

The movie records the real application from blank through export using shared
commands and command overlays. Actual projected screen rays exercise center
placement; explicit face-local coordinates subsequently remove pixel quantization
for deterministic volume checks. This is API workflow evidence. Independent Qt
widget receipts cover component activation, edge treatment, face/center clicks,
numeric input, cancellation, Repeat on a different face and Browser face views.
Physical OS clicks and an uninterrupted native-input tutorial are not claimed.

Known interaction differences remain explicit: the all-edge button replaces a
selection window; named views replace the reference view cube; an ordinary
context menu replaces the radial Repeat menu. Repeat diameter carry-over is an
Alloy choice because public instructions leave it unspecified. The source allows
Enter or OK; only OK commit is qualified in the inactive macOS Qt test window.
There is no source final-die image for a matched visual comparison.

The final binary export removes three collapsed serialized pole facets and passes
the unchanged strict reader: 53,414 facets, one closed oriented component, 20 mm
bounds and volume7801.733045 mm³ (BRep7801.394541 mm³). The separately corrected
origin-arrow obstruction was a display decoration defect, verified by origin
on/off captures; it did not change exact hemispherical geometry.

Final evidence: `build-tutorial-polish/evidence/die-final/`, 230 successful commands,
nine captured stages and exact authored reopen. The complete movie is external at
`demos/2026-10-13/die/die-complete.mp4`, 323.3 seconds with zero capture failures.
Editable model and millimeter STL are in external `deliverables/die-2026-10-13/`.
The crowded 24-feature Timeline is tracked in HIS-003/#43. Curved ASCII export
requires PRT-008/#206; this gate qualifies the binary artifact.
