# Driving rounded rectangle intent — candidate (#235)

Status: implementation candidate, 2026-10-13; independent compatibility review
pending. Public keyring source ID `warwick-initials-keyring-20261013` shows four
separate R6 sketch fillets and visible overall 60/30 labels. It does not prove
whether those labels remain driving. The researched choice for Alloy is in the
external `knowledge/specs/rounded-rectangle-driving-intent-2026-10-13.md`.

For a connected, axis-aligned, origin-based typed rectangle with four retained
side IDs and driving overall width/height, each newly authored fillet now
retargets an affected dimension witness to a surviving outer tangent endpoint
on the same support. The original dimension UUID, driving value and annotation
remain. When the origin corner is rounded, its impossible endpoint coincidence
is replaced with two explicit supporting-line incidences on the adjacent
original side IDs. These use PlaneGCS point-on-line equations and a separate
collinearity-only check; ordinary `pointOnCurve` still enforces its finite
segment. The origin point and fixed constraint remain authored. The two
incidences are deliberately internal, identified as `Support line (internal)`
in sketch point/curve inspection rather than shown as false coincidence badges.

The small `roundedRectangleIntent` record contains the four side IDs and their
original directions, width/height IDs and origin point ID. Subsequent solves
check axis supports, side ordering and positive residual lengths, quarter-arc
corner contacts, and exact overall witnesses. A side folding or oversized
radius refuses atomically. Separately authored radius dimensions remain
independent. This policy does not migrate earlier driven-span files; the
accepted issue-228 fixture still loads with non-driving spans and its original
released origin.

Focused evidence in `build/evidence/rounded-driving-intent/`:

- `rounded_rectangle_fillets` core passed four R6 fillets, R5.5 with outer
  bounds still `[0,60] × [-30,0]`, then width/height 70/35 with retained
  profile/body and the exact 2 mm extrusion volume. Oversized R30 rejected;
  undo/redo, exact save/reopen and the old driven fixture passed.
- `rounded_driving_controls` injected Qt test passed Cancel in the real width
  editor, typed `5.5 mm` for one radius and `70 mm` for width, using the
  command-produced focus throughout. `native-edited.png` shows the accepted
  labels. This is native Qt event evidence, not physical OS input.
- `tests/rounded_driving_intent_api.py` passed through the real macOS app:
  four R6 fillets, editor open/Cancel, R5.5 and 70/35 edits, atomic oversized
  refusal, analytic profile/extrusion, save/reopen. The final sketch capture
  visibly shows 70, 35, R5.5 and the other three R6 labels. The app reported
  Apple M4 OpenGL hardware acceleration; the process exited cleanly.

The command/inspection path remains the same `sketch.fillet`,
`sketch.dimensionEdit` and `sketch.dimensionUpdate` authority as native input.
Preview/Cancel for side-pair Fillet and physical side selection were established
by #228; this focused follow-up exercised actual value editing and Cancel.
The full keyring runner's new-authoring assertions now expect driving 60/30,
but no cumulative tutorial replay is claimed here. The historical #228 model,
captures and receipt describe the earlier driven policy and remain valid for
that stored model. Broader arbitrary tilted rectangles, zero-length sides and
general virtual-corner dimensions are outside this finite slice.
