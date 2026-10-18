# Regular Polygon command record

Public evidence and frame provenance: external
`../Alloy-reference-records/knowledge/specs/skt-093-polygons-2026-10-17.md`
(references P1–P4, retrieved 2026-10-17). This record describes the bounded
Alloy implementation; it does not assert exact visual or default parity.

| Aspect | Publicly supported behavior | Alloy SKT-093 state |
|---|---|---|
| Location | Sketch > Create > Polygon, then Inscribed, Circumscribed or Edge | Same named submenu; shared local commands |
| Inscribed | Center then vertex; size is center-to-vertex radius | Two-click live preview, circumradius/count/angle entry |
| Circumscribed | Center then side midpoint; size is center-to-edge midpoint radius | Two-click live preview, inradius/count/angle entry |
| Edge | Two first-edge endpoints, then side point; typed length/angle and count supported | First edge fixed in preview, opposite-side choice, typed base length/angle |
| Selection | Start in active sketch; public snap cues/relations apply | Both placement points retain supported point/origin/midpoint relations; second source owns size and angle |
| Commit/cancel | Repeat after placement; Enter exits. Exact Escape semantics not documented by polygon help | Repeat, Enter exit, Escape cancel current preview/tool; invalid input blocks placement |
| History/edit | Public tutorial shows size edit; contemporary side-count edit not established | Authored group, stable surviving IDs, count/size edit, Undo/Redo, save/reopen |
| Profile/visual | Pale filled closed regions, radial/base-edge annotation; center dots coincide with the origin in older public frames | Closed filled regions and editable true-size canvas callouts; exact styling/label placement remain unknown |

Alloy uses a visible six-side initial value and an explicit 3–128 bound. Public
references do not establish the current default, maximum, precise first-field
focus, styling or modern side-count edit behavior. [The bounded acceptance
receipt](../../validation/sketch-regular-polygons.md) separates Qt input,
geometry, GPU capture, history and printable STL evidence.
The [annotation receipt](../../validation/sketch-polygon-annotations.md) covers
native label editing and the updated visual state.
The [first-anchor receipt](../../validation/sketch-polygon-first-anchor.md)
covers native cues, actual solver relations and upstream solid propagation.
The [second-driver receipt](../../validation/sketch-polygon-second-driver.md)
covers source-owned size/angle, native second-pick cues and conflicting-value refusal.
