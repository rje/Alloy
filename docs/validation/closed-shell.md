# Closed inward Shell supported subset (SOL-048)

The candidate accepts a whole body selected in the Browser, retaining every
outer face and creating an enclosed inward-offset cavity. A canvas face still
selects the original open Shell route. The native dock labels Faces / Body,
Inside, Sharp Offset and the selected closed body. The body route clears a
previous face selection, including when switching during a Shell session.

The authored schema uses an empty removal-face object for closed mode; older
three-field face identities retain their meaning. The admitted offset kernel
constructs the cavity; a real Boolean subtracts it from the original solid.
Acceptance requires one valid solid, two closed shells, a smaller valid cavity
and exact membership of every original outer face. Inner planar supports use
the offset history and existing stable generator identities.

Focused `solid_shell` core passes exact Box50x80x40 thickness3.5 volume,
six inner supports, thickness edits, undo/redo, exact reopen and atomic failure.
Existing open Shell checks remain in the same test. A 28-call actual-entrypoint
run checks preview/Cancel, exact volume/bounds, width and wall edits, unchanged
inner face IDs and normals with updated frames, section capture and exact
support descriptors after reopen. The Qt `closedBodySelection` test clicks the
actual Browser body after a stale face selection, verifies preview, Cancel and
OK and records the readable native dock. Root inspected both GPU captures.

Evidence is external at
`../Alloy-reference-records/deliverables/closed-shell-2026-10-13/`.
Scripts: `tests/closed_shell_api.py`, `tests/solid_shell.cpp`, and the added
slot in `tests/box_shell_parameters_controls.cpp`. Delivered Qt events are
native-widget evidence, not physical OS pointer capture. The screenshot's
section clipping is display-only; the underlying model remains closed.

## Required tutorial remainder

This does not qualify the actual enclosure's grouped R5/R2.5 fillets followed
by thickness3.5. The mixed-radius blend collapse fails in OCCT offset; both
Intersection and Arc probes were unsuccessful. The concrete failing fixture
is retained as an atomic unsupported-operation check in `solid_shell.cpp`.
GEO-008/#316 owns the geometry requirement and blocks TUT-016. No smaller
thickness, omitted source fillet or open-top substitute passes that gate.
Independent Astra review accepted this bounded subset at 5b1bb29.
The actual enclosure remains blocked by GEO-008.
