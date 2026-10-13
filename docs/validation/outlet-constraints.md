# SKT-051 candidate — outlet sketch relations

2026-10-13. Public workflow contract: external `../Alloy-reference-records/knowledge/tutorials/wall-outlet-audit/independent-contract.md`, section B. This is the relation increment, not the completed outlet lesson.

`sketch.constraint` accepts `concentric` with two distinct circle/arc `entityIds`, and `horizontal`, `vertical`, or `horizontalVertical` with two ordered `pointIds`. The first selected point or curve center is an initial solve reference only. No Fixed relation is authored. The existing one-line Horizontal/Vertical route remains available. The native Horizontal/Vertical command chooses a line or a pair of points by the first viewport pick. A Line start or end click on a source-line midpoint retains an ordinary `midpoint` relation; the hover marker is a triangle. Concentric, midpoint, and point H/V relations have selectable glyphs and stable relation UUIDs.

The core fixture starts with a rough rectangle x−16..15, y13..40, turns the two sides into construction, adds a rough Ø10 circle and two outward three-point arcs through (−25,26.5) and (24,26.5). The arc endpoints reuse rectangle corner UUIDs. Concentric left→right, point H/V circle center→left arc center, and Equal left→right solve in that order. No final cutout coordinate or radius is pinned. The same test checks circle/arc and circle/circle Concentric, reverse point order, both Line endpoint midpoint snaps, a source-width edit moving the snapped endpoint, JSON solve roundtrip, duplicate refusal, an unrelated redundant Equal refusal, and incompatible fixed-center refusal.

The exact rectangle/arc fixture makes one coordinate of Concentric rank redundant. PlaneGCS attributes that rank cycle to a preexisting vertical-side relation, not necessarily to the new Concentric ID. The bounded policy accepts a zero-conflict successful solve only if removing Concentric from the same authored system yields a clean solve. Both Concentric equations remain present, and independent geometric residuals must still pass. Diagnostics retain `redundantConstraintIds` and add `acceptedConcentricDependentRedundancyIds`; unrelated redundancy and conflicting centers continue to refuse atomically.

Focused native Qt input exercised actual midpoint hover and two pointer clicks, followed by arc Concentric picks, circle-center then arc-center H/V picks, and Equal picks. Tool activation, selection clearing, and construction conversion used the shared API in the Qt fixture; they were not native palette clicks. The settled capture and authored state are outside the repository at `../Alloy-reference-records/evidence/outlet-constraints-2026-10-13/` (relative to the main checkout). They show 8 edges, 6 remaining DOF before the later tutorial dimensions, 9 constraints, and a maximum independent residual of 5.33e−15 mm. The GPU state reports Apple M4, OpenGL 4.1 Metal, native framebuffer 872×671, physical framebuffer 1744×1342. The captured sketch is deliberately rough and underconstrained; full reflected geometry, dimensions, and zero-DOF tutorial gate remain separate work. Constraint glyphs are visible and have stable pick bounds in the captured state; a badge click was not part of this native run.

Independent visual review found the first Concentric square/crosshair at both arc midpoints mismatched the public `a2-concentric.png` reference glyph. The corrected original badge draws two rings once near the shared solved center. A focused settled GPU capture shows the rings; its 12×13 logical-pixel hitbox at x411/y245 does not overlap the point-Vertical hitbox at x431/y245 or any other badge. The public image is a small instructional crop with a different viewport scale, so this is a shape/placement match, not a pixel match.

Validation: `outlet_constraints_test` passed after the final projection-lock preservation guard; `outlet_constraints_controls_test` passed again (3/3, 8.1 s) after the glyph correction. The guard only excludes external projected support points from authored point serialization while an initial reference point is temporarily held; the native fixture uses authored points. The private editor closed after capture. Native pointer and API checks share the production `AlloyWindow` commands and retained `SketchGeometry` data; the core fixture is independent of viewport picking.

## Independent acceptance

Accepted `2ad0bd368067cc8951537171fdfb8d7d8aad333a` atop `e6814e9` on
2026-10-13. Independent review covered the implementation, exact source-like
rectangle/opposed-arc core fixture, persisted core output, corrected native log
and actual GPU capture. No additional validation editor was opened.

The redundancy exception requires a successful solve without conflicts, a
clean solve of the same system with Concentric removed, and successful
independent residual checks of the full retained system. It preserves the
reported redundant IDs rather than silently discarding authored relations.
The fixture exercises the real dependent center-height equation; duplicate
Equal, duplicate Concentric and incompatible fixed-center refusals remain
covered. This accepts the bounded dependent-equation policy, not a general
promise to accept arbitrary redundant systems. Temporary first-point locking
is used only to establish initial selection-order placement, then the system
is solved again without that lock. No hidden Fixed relation is serialized.
The final external-support guard preserves the preexisting projection filtering.

Midpoint endpoints retain source-line references and follow an upstream edit.
The actual Qt test covers midpoint hover/two clicks and the Concentric, point
H/V and Equal pair clicks. Fixture geometry, tool activation, selection clearing
and construction conversion use shared commands; this is not a native menu or
Shift-selection claim. It introduces no focus repair between the demonstrated
pair picks. The corrected run records 3/3 in 8.101 seconds.

The first review rejected the square-crosshair Concentric badge. The corrected
capture now shows one recognizable, independently authored pair of rings near
the shared center, separate from the point-Vertical badge and midpoint markers.
The focused assertions check one stable relation badge and noncolliding bounds
in this fixture. Labels and palette are readable. Badge clicking, arbitrary
layout collision avoidance and physical OS input remain unverified. Six DOF
are truthfully reported for this rough fixture; mirrored/dimensioned zero-DOF
completion remains the cumulative tutorial gate.
