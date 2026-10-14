# Extrusion base boundaries and midpoint ellipse

DOC-026/#311, 2026-10-13. Independently accepted and integrated. This ingredient supports the source's front-Lid thumb sketch.

Straight base-cap boundaries use exact prism FirstShape history and membership,
base-specific stable identities and verified source endpoint orientation.
Existing base face identities/frames remain intact. The focused Offset-start
core test verifies actual base membership, four unique rectangle edge IDs,
separation from end-cap IDs and stable IDs through signed starts, reversed
extent, start edits and reopen. Existing ellipse core checks also pass.

Ellipse authoring now accepts an actual authored/projected line midpoint through
the existing retained Midpoint constraint. A runtime capture exposed a second
problem: projected-profile extraction stripped the ellipse's three structural
axis relations and consequently refused its profile. Extraction now preserves
those intrinsic midpoint/perpendicular relations; authoring is unchanged.

`tests/base_cap_midpoint_api.py` creates a front-facing stock, projects its actual
base boundaries, snaps a fully constrained 15×2 ellipse to the top midpoint,
changes source width50→74 and verifies center25→37, stable edge IDs and reopen.
It then cuts the actual conic regions with Start Offset−10 and Distance5.
Removed volume58.904862254807 mm³ matches the half-ellipse analytic expectation
58.904862254809 mm³. Captures and ledger are external at
`deliverables/base-cap-midpoint-2026-10-13/candidate-03`.

This is a rectangular-stock ingredient, not the completed dovetail-Lid gate.
Curved base boundary projection and physical OS input are not claimed. The final
Cut capture retains blue body selection; cumulative neutral captures remain
part of the complete tutorial gate.
