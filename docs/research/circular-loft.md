# Ordered circular Loft contract

Bounded SOL-026 / #190 follows stable reference tutorial-egg-cup-173, source steps
12–13 and the four earlier sections. Public source evidence and screenshots are
external in the indexed tutorial ledger. The documented inputs are circles
Ø38/16/30/44 at z0/18/38/58 mm, selected in that order, Free endpoints, no rails,
Chain Selection on, Closed off, New Body. Exact spline interpolation is unknown.
The source next removes the top face with an inward 2 mm Shell.

`solid.loft` and `solid.loftDialog` take `profiles: [{sketchId, profileId}, ...]`,
optional `name` and optional `featureId` for editing. The shared authoritative
candidate creates one retained Loft feature/body; its parameters retain ordered
profile references. This slice accepts 2–32 parallel coaxial analytic circles in
strictly monotonic axial order. Rails, mixed profiles, Join/Cut, closed lofts and
other endpoint conditions remain in parent #58.

The native panel accepts viewport or Browser profile selections in click order;
a repeated selection removes that section. Its numbered list offers Move Up,
Move Down and Remove. Name, real GPU preview, OK/Cancel and Escape use the same
session and evaluated document as automation. The preview reserves stable feature
and body IDs for repeated previews/commit. Timeline editing preserves those IDs,
resolves against earlier history and previews the Loft before downstream Shell.
Source sketch or plane edits recompute the same references; unavailable references
and invalid ordering fail atomically.

OCCT 7.9.3 `BRepOffsetAPI_ThruSections` creates a solid with ruled=false, immutable
section inputs, compatibility checking, C2 continuity, max degree 8 and smoothing
optimization disabled. These are Alloy interpolation decisions, not claims about
reference internals. Kernel FirstShape/LastShape provenance names two planar caps
with stable Loft feature/endpoint UUIDs; cap selection supports downstream Shell.

Library evidence: [OCCT through-sections documentation](https://occt3d.com/dev/doc/refman/html/class_b_rep_offset_a_p_i___thru_sections.html),
retrieved 2026-10-13, plus the installed open source 7.9.3 API declarations.

`solid_loft` verifies real plane intersections at all four exact circular section
radii, a curved intermediate section differing from ruled interpolation, valid
closed one-solid geometry, the exact 2 mm Shell, ordered-reference refusal,
diameter/offset recompute, stable body/rim IDs, undo and reopen. The real-entrypoint
script `tests/solid_loft_api.py` additionally exercises selection.pick while visible
construction planes exist, the ordered panel/preview/cancel/commit, editing and
captures. Runtime and independent source-like silhouette review are required
before acceptance; core tests alone do not establish visual compatibility.
