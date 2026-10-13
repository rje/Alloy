# Complete slots and spline lesson gate preparation (#257)

Prepared 2026-10-13. **Not executed or accepted.** Source ID
`sketch-slots-splines-20261013` covers all31 steps of the public13-page lesson.
Detailed provenance, images, discrepancies and design reviews remain external in
`knowledge/tutorials/after-countersunk-plate/independent-contract.md` and its
companion command/design records. Children #251–256 are required prerequisites.

## Source-order creation record

Start a blank metric document before recording; finish with three saved sketches.
The shared-command movie must include creation and demonstrated edits, not just
loading finished fixtures. Native ingredient evidence remains separate from API
movie evidence. Do not claim physical input or pixel parity from either geometry
assertions or API captions.

| Stage | Required source actions | Evidence |
| --- | --- | --- |
| Straight slot | XY sketch; rough center/direction/width picks at origin; Escape; underdefined point drag; centerline H/V; D150 full centerline and R25 | Retained midpoint/origin intent, genuine drag, tangent shape, readable constraints/dimensions and final0DOF |
| Arc slot | Origin center, two end centers and width; path R150/endR15; horizontal endpoint relation; radial Line to origin; Escape; X construction; D135 against first slot centerline | Upper90degree branch, retained references, two real closed regions, readable angle/radius witnesses and final0DOF |
| Sketch transition | Finish Sketch; Browser hides first sketch; new XY sketch | Correct sketch ownership and visibility |
| Fit spline | Three arbitrary fit picks; checkmark/Escape; show handles; delete middle point; Insert Spline Fit Point through curve selection; drag tangent grip | Stable fit identity, evaluated through-point curve, actual handle direction/weight change; no fixed polyline substitute |
| Control spline | Three control picks; checkmark/Escape; control cage drag | Three authored cage vertices, evaluated actualdegree2, changed real curve |
| Curvature | Select spline; enable palette Curvature Comb; inspect; Finish Sketch | Derivative-based comb with readable cage/handles; actual sampling rather than drawn imitation |
| Conic | Hide second sketch; new XY; endpoint/end/vertex; change Rho; right-click OK | Actual editable conic and correctly associated Rho manipulator; inferred mapping disclosed |
| Ellipse and Save | Center/axis/axis picks; Escape; constrain/dimension horizontal origin ellipse275x100; Finish and Save | Retained center/orientation and driving full axes; final third sketch visible, first two hidden |

No solid, STL or printing step appears in this source. Adding an extrusion cannot
replace a missing sketch command. Arbitrary fit/conic coordinates should be
explicit, well-conditioned fixture choices. The ellipse's precise constraining
actions are omitted by the source prose; record the Alloy steps needed to match
its pictured final intent as adaptations. Do not assert that open conic and
splines are fully defined because the ellipse or first sketch is fully defined.

## Geometry and integrity oracles

The straight slot has arc centers(-75,0)/(75,0), radius25, envelope200x50 and
area `7500+625*pi`. Its150 dimension is full centerline length; the creation
field's half-length has a different meaning. The first-click origin prose is
inconsistent with its command and illustration; use the independently reviewed
center-first interpretation and disclose that discrepancy.

The arc slot has path radius150, end radius15, upper endpoint centers
`(+/-150/sqrt(2),150/sqrt(2))`, side radii135/165 and quarter-circle sweep.
Area is `2475*pi`; it is one rounded band region without an inner hole.
Together the first sketch has two separated regions. Check actual analytic
curves/profile areas and retained constraints, not just matching bounding boxes.

For splines, verify ordered authored points, actual OCCT curve, through-point
residuals where applicable, and demonstrated edit effects. Actual control degree
is distinguished from requested maximum. Reuse the accepted evaluator's
nonuniform-span continuity evidence; the cumulative gate need not repeat its
entire numerical test suite. Comb and handles require actual GPU inspection.

The final ellipse itself must have0DOF. It has semiaxes137.5/50, area `6875*pi`, and bounds
`[-137.5,-50]..[137.5,50]`. Conic Rho examples in the source pictures are not
mandatory exact final Rho values. Record the fixture and qualified geometric
mapping; verify edited rho, endpoints and persisted intent.

Save the source endpoint before supplementary integrity checks. Reuse scoped
child receipts for representation-specific edge cases. In the cumulative gate,
make one meaningful retained dimension edit in the first sketch and one curve
edit, verify exact undo/redo, then reopen the saved source file and compare
three-sketch authored identity/visibility and evaluated geometry. No arbitrary
face-count or screenshot-only success condition.

## Review and outputs

Keep actual model, full creation movie, structured report, stage captures and
original failure evidence in the external deliverables folder. Review first-sketch
constraints, fit handles, control cage/comb, conic arrow/value and final ellipse
witnesses for readable identities, contrast and association. Compare permitted
source views where scale/state permits and disclose unmatched conditions.
Independent review must cover every source action and artifact before issue257
or its board card becomes Done. Intermediate children do not accept the lesson.
Only then add the full lesson to the external gallery.

Independent preparation review checked this ledger against all31 source steps
and found no material omission. This approves the planned coverage only; no
feature or cumulative runtime acceptance follows.
