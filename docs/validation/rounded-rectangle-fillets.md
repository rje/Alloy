# Rounded rectangle overall spans — candidate

SKT-043 / #228, 2026-10-13. Public source ID
`warwick-initials-keyring-20261013`; provenance, all six page images and the
independent contract remain outside this repository in
`../Alloy-reference-records/knowledge/tutorials/warwick/initials-keyring-independent-contract.md`.
The demonstrated step is four successive Sketch Fillet side-pair selections at
radius 6 on a 60 × 30 two-point Rectangle. Its image retains 60/30 annotations;
whether those remain driving after filleting is unknown.

Alloy uses the accepted two-point Rectangle sketch and its four original line
IDs. The existing fillet authority creates one real quarter arc per selected
adjacent side pair. It keeps the 60/30 dimension IDs as point-pair **driven
overall references**, remapping a retired corner endpoint to the tangent point
on the perpendicular outer side. It never labels the shortened horizontal or
vertical straight portions 60/30. Before acceptance, the preview notice names
the reference conversion and the retired corner-origin coincidence. The fixed
origin marker survives, but after rounding its corner, the contour is no
longer anchored to that marker. The final sketch has four degrees of freedom;
no virtual corner or unclaimed driving-size constraint is introduced.

The fillet tool now accepts a click on one side followed by a click on its
adjacent side as a single-corner candidate. The earlier click-corner twice
mode remains for a shared two-corner radius. In the new mode, the radius field
previews a disposable candidate; Enter accepts and Escape/Cancel discards it.
The shared `sketch.filletPreview`, `sketch.filletAccept`, `sketch.filletCancel`
and direct `sketch.fillet` commands retain one authority. Invalid geometry or
unrelated references refuse through the existing atomic fillet solver.

Focused core `rounded_rectangle_fillets_test` passes four sequential R6 edits:
four source lines plus four real quarter arcs, unchanged line/dimension IDs,
one closed profile, measured 60/30 outer spans and analytic area
`1800 − 36(4 − π) = 1769.0973355 mm²`. The existing
`shared_line_fillets_test` also passes. One supported final radius edit from
6 to 5.5 retains the dimension IDs and their outer-boundary measurement, but
the underconstrained solver changes the actual bounds to about
59.6999 × 29.6996 mm. The reference labels report that changed measurement;
Alloy does not claim to preserve the original 60/30 size through radius edits.

The focused native Qt case `rounded_rectangle_controls_test` passed 3/3 in
4.606 seconds. A shared-command two-point Rectangle seed is followed by real
viewport Qt clicks on four successive side pairs, typed radius 6 and Return.
The last preview is canceled without draft change, picked again and accepted.
This establishes Qt event delivery to the real GPU view, not physical OS
input or the Rectangle's already-accepted native typing sequence. The final
actual preview was inspected: `(60)`, `(30)`, four `R6` labels, the full origin
warning and the radius field are readable. A dynamic notice-height correction
removed clipping; its text now occupies 90 logical pixels at this viewport.
Evidence: `build/evidence/rounded-rectangle/native.log` and
`rounded-preview.png`.

The real executable `tests/rounded_rectangle_api.py` passed command-first
Rectangle authoring, four viewport-space side-pair picks, R6 preview/Cancel,
exact analytic area, radius edit, undo/redo and authored save/reopen. It
records the saved model, accepted capture and report under
`build/evidence/rounded-rectangle/api/`; its process exited cleanly. API
viewport coordinates are not native pointer-input evidence.

Compatibility status is **matched** for the bounded side-pair sequence,
rounded outline and visible outer 60/30/R6 annotations in Alloy's tested
fixture. Post-fillet driving behavior and exact reference styling remain
**unknown**. The distinct retained-driving overall-size capability and the
complete keyring lesson remain outside this issue.

## Independent acceptance

Accepted candidate `959a72ee` for #228 on 2026-10-13. Independent review covered
fillet remapping/retirement and side-pair session code, the core and native test
sources and persisted passing logs, API report/saved authored data, and actual
preview capture. The saved model exactly equals the report's authored object;
both original dimension IDs remain non-driving point-pair spans without a
stale valueMm, measuring actual outer 60/30 bounds rather than trimmed sides.
The radius-edit test explicitly compares their new measurements to the changed
boundary bounds. It does not establish size invariance or origin anchoring;
that capability is retained separately in #235.

The preview clearly shows (60), (30), four R6 labels, and the complete notice
that conversion occurs and the outline loses its origin anchor. The fixed
origin marker is not misrepresented as an attached contour constraint in this
receipt. This candidate capture predates the separately accepted #234 tangent
badge correction and still contains the old outline-box badges; reuse #234's
corrected actual capture/source acceptance for that unchanged presentation path.

The native case uses real viewport side-pair clicks and typed radius/Return,
asserting natural focus on the radius editor without in-sequence repairs.
Its rectangle/tool setup and preview Cancel use shared API commands; it is not
claimed as native Cancel-key or physical OS evidence. The final one-corner
Accept guard merely requires the sidePair state already asserted by that
passing case and preserves the older two-corner route. Source inspection and
its successful rebuild are sufficient for that post-capture guard; no unchanged
replay was needed. This accepts the demonstrated rounded-outline/annotation
slice, with the disclosed reference conversion, not full source solver parity
or the cumulative keyring lesson.
