# Partial capability rows in the printable-part v1 plan

Implementation baseline, 2026-10-18, following the user's review of the v1
direction; the first-15 scope proposal below was added 2026-10-19. These are
the retained minimum **proposed v1** variants, not
acceptance of each full command family or physical-input parity. The
accepted receipts prove only their named subsets. REL-030 still owns the final
inventory, release-gate and site reconciliation.

## First 15 broad partial proposed-v1 rows

The following boundaries are a proposed minimum for a robust creation tool:
users can start and recover a design, author and constrain editable geometry,
and repair or clearly refuse broken dependencies. They are scope proposals,
not feature acceptance. Existing receipts establish only the accepted slices
named in the reconciliation table; unresolved behavior remains proposed until
REL-030 closes the inventory review.

| Inventory row | Proposed minimum for v1 | Later or separately owned remainder |
| --- | --- | --- |
| `document.lifecycle` | New, Open, Save, Save As and multiple design tabs for native authored documents, including dirty-close handling and safe refusal of unsupported newer schemas. | Older-schema migration and interrupted large-save recovery under REL-002. |
| `document.recovery` | Recover the latest committed authored state after interruption; a failed Save must preserve the previous valid file. Active tool/sketch drafts may be excluded. | Schema migration and broader large-save/disk-full recovery under REL-002. |
| `parameter.user` | Named length parameters, arithmetic dependencies, and direct bindings for supported sketch driving dimensions and feature distances; reject invalid dependencies without losing the last valid model. | General functions, all unit types, arbitrary feature fields, and broader rename/dependency semantics. The exact retained contract remains proposed under PAR-001. |
| `history.edit` | Edit an existing feature input and recompute downstream features while preserving references, with undo and save/reopen. | Broader feature edit/reorder coverage; dependency-validated reorder has its own accepted slice in HIS-006. |
| `history.suppression` | Suppress and restore one feature, propagate the dependent state, and retain the authored intent through undo and save/reopen. | General suppression across every feature/dependency combination; behavior remains proposed under HIS-003. |
| `history.rollback` | Temporarily evaluate an earlier history point and return to the latest feature without deleting authored future history. | Persisting a non-latest marker and its save/reopen policy; those choices remain proposed under HIS-003. |
| `history.reference-repair` | Preserve common downstream references through edits; for a missing or ambiguous reference, show an explicit repair path or refuse atomically without arbitrary rebinding. | All topology-change classes, projected-link variants and broader repair UX beyond the accepted named cases. |
| `sketch.lifecycle` | Create, edit and finish origin-plane sketches and sketches on supported retained planar faces; allow explicit redefinition for the qualified ordinary support route. | Broader face/support types, normal-entry preferences and topology-repair cases. |
| `sketch.interaction` | Native line and rectangle creation with typed values, visible supported snapping/inference, and explicit conflict recovery that preserves the draft. | General intersection inference, all inline-key/focus combinations and broader constraint conversion. The final minimum is still proposed under SKT-021. |
| `sketch.line` | Create chained lines with coordinate or length/angle entry and retained intent; support the accepted endpoint, horizontal/vertical and tangent inference slices. | Other inference families and remaining native numeric-entry paths. |
| `sketch.fit-spline` | Editable fit-point splines with retained tangent handles and downstream profile use. | Periodic/closed variants, broader endpoint constraints and other spline modes. |
| `sketch.control-spline` | Editable control-point splines with curvature inspection. | Pole/weight/continuity options beyond the accepted control-point slice; exact scope remains proposed under SKT-012. |
| `sketch.construction` | Construction geometry excluded from profiles, supported midpoint relations, and point-to-edge distance dimensions with driving/driven behavior. | Additional relation families and unsupported curve/edge combinations. Point-to-edge behavior remains open under SKT-016. |
| `sketch.dimensions` | Driving and driven dimensions for supported lengths, angles, radii/diameters and common point/line distances, with unit-aware display and retained intent. | Broader dimension types and combinations. Point-to-edge distance and offered driving-to-driven conflict conversion remain proposed/open. |
| `sketch.project` | Associative projection of supported planar edges, vertices and circular references into a sketch, with source identity retained and safe failure when a source disappears. | Independent-copy choice, broader analytic curves/silhouettes and explicit relink/break-link repair beyond the qualified linked cases. |

The minimums above intentionally overlap where one workflow depends on another:
construction relations support dimensions, projection supplies sketch
references, and history repair preserves downstream features. Release evidence
must still exercise the shared UI/API authority, actual geometry, edit, undo,
save/reopen and native input for the required subset. These proposals do not
close the open owners or cumulative release gates.

| Inventory row | Proposed v1 subset | Wider family after v1 or separately owned |
| --- | --- | --- |
| `sketch.arc` | Center Point, Three-Point and accepted line hold-drag tangent continuation | Other tangent modes and construction variants |
| `sketch.slots` | Center Point, Center-to-Center, Overall and Center Point Arc slots | Broader endpoint/constraint editing and other input routes |
| `sketch.ellipse` | Center Point Ellipse with editable axes | Other construction modes and constraint combinations |
| `sketch.conic` | Three-point conic with editable Rho | Broader constraints and downstream combinations |
| `sketch.text` | Editable outline text in the accepted font subset, with placement and rotation | More fonts, path text and layout |
| `sketch.fillet` | Accepted one- and multiple-line-junction fillets, including shared-line and dimension treatment | Analytic/spline junctions and wider options |
| `sketch.mirror` | Associative line/arc and ordinary open fit-spline Mirror | Closed/control/offset splines and transformed support |
| `solid.primitives` | Box, Cylinder and Sphere, including accepted Sphere Cut/Repeat | Other primitive commands retain their separate owners |
| `solid.combine` | Explicit-target Join/Cut/Intersect, Keep Tools and ordinary invalid-result recovery | SOL-071/#566 accepts bounded names/recovery; broad topology remains later work |
| `body.rename` | Current local body via Browser/shared command, retaining identity | Imported/nonlocal organization policy; physical gestures stay in release input gates |
| `construction.midplane` | Retained plane between supported parallel planar faces | Nonparallel/angular-bisector cases |
| `construction.path-plane` | Start/End plane on supported retained fit-spline path | Arbitrary fractions and other curves |
| `construction.tangent-plane` | Tangent plane at a retained point on a supported face | GEO-004's bounded constructor is accepted; GEO-024/#590 retains the ordinary native-input route, and broader face/point cases stay later |

Issue ownership and accepted evidence are in [the ownership audit](ownership-audit.md),
the [inventory](command-inventory.json) and their linked receipts. Release
qualification must also cover actual native interaction, geometry, edit,
undo/reopen/export and packaged runtime on each supported platform. A proposed
subset is not a release promise until REL-030 and the cumulative gates accept it.
The first-v1 3MF requirement means Core **export**; import and optional
extensions remain with XCH-005 on the noncloud roadmap. Modeled external Thread
is required in the first-v1 printable workflow; internal/tapped Hole variants
remain separately owned and are not implied by that slice.
