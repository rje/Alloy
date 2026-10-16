# Separate profile bodies from Surface Extrude

SRF-020/#461 is independently accepted. Implementation `e690833`, legacy wording correction `6e0be0c` and picker/
label correction `fc3d475` implement one Two Sides surface extrusion with separate
bodies per explicitly selected open profile. Astra independently accepted source, workflow receipts and actual GPU captures.

Public operation frames established two selected profiles, Chaining enabled,
Two Sides/New Body and two separate resulting tool bodies. Exact curve grouping
and dimensions are not established. The Alloy fixture explicitly chooses an upper
spline/line chain, an independent lower spline and later an overrun bottom line.
Source records and frames remain outside Git.

New `perProfile` features retain explicit profile memberships. Connected,
nonbranching open chains use actual OCCT wire/prism construction and generated
face lineage. Body identity derives from the feature, sketch and canonical source
membership, not selection order. Every output participates in ownership,
dependencies, selection and saving. Existing absent-mode features retain their
original compound result and body identity; edits do not silently migrate modes.

## Evidence

Focused core and legacy tests pass. Source/reach/order edits preserve identity;
adding a third profile preserves the first two results. The core checks also
cover Remove/history/reopen, unavailable-reference refusal and independently
addressable outputs as tools for the existing planar Trim ingredient.

The actual accepted iron bridge fixture produces two new bodies in its component.
Editing the same sketch and Extrude adds a third body while keeping prior face/
edge IDs. This makes five bodies including the existing exterior and bridge.
The 45-call API replay passes preview/Cancel, commit, all-result feature editing,
third-profile addition, undo/redo and exact saved reopening.

Native Qt input passes 3/3: visible Surface Create menu, viewport chain/profile
picks, Chaining, typed Side 1/Side 2 distances, preview/Cancel, commit and edit/Cancel.
Final actual Apple M4 GPU captures show both sheets, readable Profile 1/2 curve
counts and smooth geometry. Initial API picker failures exposed optional JSON
null insertion and an incorrect sketch-only feature lookup; both were fixed in
the controls without changing or repeating accepted kernel tests.

Evidence is external under
`../Alloy-reference-records/deliverables/surface-iron-continuation-2026-10-15/per-profile-extrude-01/`.
The next-stage saved input is `native/native-two-profile-sheets.alloy`.

## Limits

Five bodies do not prove a closed cell. The current added bottom line lies at
Z=−4 below the exterior's Z=0 lower boundary; final closure needs an explicitly
chosen and verified geometry continuation. Curved Trim trials, Boundary Fill,
solid export and the complete TUT-018 recording remain open.

Cold picks take 9–11 seconds, commit 10–12 seconds, edit opening 18–22 seconds and
adding a profile about 26 seconds in this fixture. REL-016 records this measured
responsiveness gap. This gate establishes functionality, not useful interactive
latency or Windows/Linux runtime qualification.
