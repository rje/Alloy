# Partial capability rows in the printable-part v1 plan

Working disposition, 2026-10-18. These are minimum **proposed v1** variants,
not acceptance of each full command family or physical-input parity. The
accepted receipts prove only their named subsets. REL-030 still owns the final
inventory, release-gate and site reconciliation.

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
| `construction.tangent-plane` | Tangent plane at a retained point on a supported face | GEO-004 retains complete native interaction; broader face/point cases stay later |

Issue ownership and accepted evidence are in [the ownership audit](ownership-audit.md),
the [inventory](command-inventory.json) and their linked receipts. Release
qualification must also cover actual native interaction, geometry, edit,
undo/reopen/export and packaged runtime on each supported platform. A proposed
subset is not a release promise until REL-030 and the cumulative gates accept it.
