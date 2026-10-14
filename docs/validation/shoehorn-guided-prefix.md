# Guided shoehorn sheet: accepted API prefix

Independent review accepted this bounded prefix on 2026-10-14. The real
application starts blank, authors a YZ fit spline with driving 130/50/16 mm
dimensions, creates endpoint-normal planes, constrains the front arc to
50/10/10 mm and the rear line to 30 mm, then creates a guided surface Loft
in a new component. This is an open sheet, not a printable finished shoehorn.

`tests/shoehorn_workflow_api.py` is the reproducible runner. External evidence
is `deliverables/shoehorn-workflow-2026-10-14/prefix-02/` in the reference archive.
All 65 recorded commands passed. The 16→18 mm bend edit changes surface area
from 5600.469238 to 5648.220066 mm² while retaining the face identity. Authored
undo, redo and saved-variant reopen match exactly. Five actual-app captures
show readable dimensions and the coherent evaluated sheet.

The first fit point is fixed at the origin and the end points are aligned
horizontally as explicit Alloy anchoring choices; the public lesson does not
establish those exact constraint gestures. The inspected cyan sheet is a known
presentation mismatch tracked by VIS-003. Public sources and visual references
remain outside the repository under the dated surface tutorial contract.

This receipt excludes native creation gestures, movie acceptance, Thicken,
corner/perimeter finishing, tangent-plane hole placement, final organization
and printing export. TUT-017 remains open. GEO-018 preserves the generated
Fillet face/endpoint references required by the eventual hole support. The
current Thicken candidate's source consumption and component ownership are
an explicit adaptation until public evidence resolves the final body-move step.
