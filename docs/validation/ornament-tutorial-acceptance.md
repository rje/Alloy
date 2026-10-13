# Ornament cumulative acceptance

Independent Astra review, 2026-10-13, #264 and #269. Accept the bounded
source-workflow tutorial and corrected binary STL export runtime. This is an
aggregate of preserved creation evidence, an independent saved-model geometry
check and targeted corrected exports; it is not a claim that the original
cumulative-05 process passed every stage.

External evidence root: `deliverables/ornament-2026-10-13/` in the reference
records. Source provenance and images remain under stable public reference
`ornament-pattern-20261013`. The reviewer inspected the original and edited GPU
captures, public images, movie storyboard/final frame, report/transcript,
geometry followup and re-export metadata without operating a reference product.

## Accepted workflow and geometry

`cumulative-05` records blank-document creation in source order: dimensioned
80/5/R50 radial profile, Full Revolve, selected-bottom-face Inside Shell5,
hatched XZ Section Analysis, exact fit-spline Offset5 with explicit endpoint
lines, Intersect, eight retained patterned bodies, diameter10 downward5 hub Join
and diameter4 transverse bore. The original and offset-4 models are editable.
The 127-call transcript includes pre-Join count8→10→8→10→8 history and final
source-offset edit, undo/redo, save and original/edited reopen. Explicit Join
participants do not automatically expand with a later count increase.

`geometry-verifier-followup.log` reports geometry-pass for both saved models:
one BRep/Boolean-argument-valid body with retained identity, all nine member/hub
material-preservation residual volumes zero, stable oracle input/result hashes,
closed-tip axis material, seven bore-axis outside samples, eight adjacent-wall
inside samples and three pre-Cut axis inside samples. Original and edited solid
volumes are 18838.785778245023 and 15579.268623588747 mm3.

Actual captures show readable dimensions, distinct strip curves and closure,
whole-base hatching, eight retained bodies, and a smooth final ribbed ornament
with hub/bore. Both original and edited results retain the closed pointed apex;
the earlier cumulative-04 open-crown mismatch is resolved. The 82.6-second,
5fps creation movie contains 413 frames, 337 repeated frames, 63 delayed ticks
and zero capture failures. It records real API-driven application/GPU output
and waiting time; it does not establish a native mouse/keyboard tutorial replay.
Accepted native ingredient evidence remains separately scoped.

## Corrected exports and preserved failure

The original `cumulative-05/report.json` remains failed at strict edited STL
normal validation. Its two initial meshes and failure evidence are preserved.
The independent geometry followup and `reexport-01` are separate evidence;
acceptance does not relabel or deliver the failed edited mesh.

After #269 candidate `7b18282` (integrated `794f3fe`), `reexport-01/report.json`
reports reexport-api-pass. The nine-call actual-app transcript opens both saved
models, inspects, exports, reinspects and quits successfully. The app SHA256 is
`02f274afb233515b6dff076f520d05a32348a3c649abcaf28fda2f47b1a7b0d2`.
Both strict checked binary millimeter meshes have one closed oriented component,
positive volume and finite unit normals aligned with winding: original36952
facets and edited33410. Mesher status is zero; authoredChanged is false.

Independent byte comparison confirms both saved-source hashes match the report,
and corrected exports retain every original facet coordinate/attribute byte and
facet count. Only normals differ. Deliver these corrected files:

- `reexport-01/original-mm.stl`, SHA256
  `d6efbcc651cc72d157c28b326c2da38fc3cfff2b0267ce4d98f70a0df94abb6c`.
- `reexport-01/edited-mm.stl`, SHA256
  `7e2c7a858e362798f83edecc5a85f11231bb122d19dab82840b2684b4e7d780a`.

## Scope and limits

Free fit points, signed offset side, Intersect reach, explicit line closure and
numeric hole placement are disclosed fixture choices. Exact source spline
coordinates and lower shell boundary remain unknown. Public and Alloy camera,
scale/material are unmatched, so acceptance is qualitative workflow/silhouette
compatibility, not exact pixel/curve parity. The optional printing brim is
excluded; checked STL is not a physical print or cross-platform runtime claim.
No remaining blocker was found for this bounded tutorial gate or #269 runtime.
