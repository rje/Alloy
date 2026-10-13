# Straw bung workflow gate

Research contract dated 2026-10-12; execution remains pending under
[TUT-004 #170](https://github.com/rje/Alloy/issues/170).
Source ID `tutorial-straw-bung` identifies the public illustrated lesson;
`bung-original-video-20261012` identifies its recovered original video.
The detailed 14-step ledger, provenance, source variants and inspected frames are
external at `../Alloy-reference-records/knowledge/tutorials/warwick/straw-bung-ledger.json`.

## Required modeling sequence

1. From empty document, Center Diameter Circle, XY plane, origin, diameter26 mm,
   Enter, Finish Sketch. Source command precedes support selection.
2. E, circular profile, 3 mm, New Body, OK.
3. Circle command, select positive top cap, origin, diameter19 mm, Enter, Finish.
4. E, circular profile, 8 mm, Join, OK. One stepped solid; base diameter26/depth3,
   boss diameter19/depth8, total height11 mm.
5. Circle command, select the **Join-created** positive top cap at z11, origin,
   diameter6.2 mm, Enter, Finish. E, profile, tall New Body cutter, OK.
6. Right-click cutter, Move/Copy, Bodies, Free Move, rotate by10 degrees using
   arc handle, then move downward through stock using axial arrow; OK.
7. Modify > Combine, Cut; target bung, tool cutter; Keep Tools unchecked; OK.
   Final result is one stepped solid with one angled through-hole.

Do not replace sketches with cylinder primitives, Join with a separate Combine,
Move with a pre-positioned cutter, or Combine Cut with an extrusion cut.

## Source variants and explicit uncertainty

The PDF shows cutter60 mm and X Angle-10 degrees; its2 mm Y translation screenshot
is an intermediate state and does not establish final downward distance.
The original video shows cutter40 mm, Y Angle+10 degrees at05:07 and Z Distance
-20 mm at05:17. At05:17 the angle fields are zero while the cutter stays tilted:
Free Move accumulates successive gestures. Keep these two variants distinct.
A PDF-based fixture may explicitly adopt20 mm downward travel from the video;
it must label that adaptation, never claim an observed exact PDF final transform.

The gizmo pivot is visibly at the cutter foot; mapping this to(0,0,11) is an
inference from the authored dimensions. Downward translation appears along the
rotated local cylinder axis; exact internal transform composition is unknown.
Declare Alloy's pivot/axis/composition convention and inspect actual geometry.
Official public reference distinguishes Free Move from parameterized Translate;
Alloy persistent editable rigid transforms are a product requirement, not proof
that six permanent Euler parameters match the reference's history semantics.

## Missing capabilities and owners

- [#168](https://github.com/rje/Alloy/issues/168): retained one-target Extrude Join,
  positive result cap lineage, next face sketch, prefix-history editing and
  downstream recompute. Keep target body ID and cap producer identity distinct.
- [#169](https://github.com/rje/Alloy/issues/169): persistent body Move,
  selection and pivot, sequential translation/rotation preview and native gizmo.
  Move the evaluated body, preserving upstream sketch and dimensions.
- [#175](https://github.com/rje/Alloy/issues/175): Circle-first plane/face entry is presently an explicit interaction gap: the
  existing C shortcut only activates in a sketch. Separate Create Sketch first
  is an adapted route and cannot pass exact source steps1/5/7.
- Existing Combine must consume the moved evaluated cutter, retain target ID,
  and consume tool when Keep Tools is false. Qualify this chain on the candidate.

## Bounded acceptance

Run one blank-document API chain plus native evidence for circle-first support
selection, inline diameter/Enter, E/operation changes, right-click Move/Copy,
rotation arc and translation arrow, and Combine target/tool picking. Record each
source step as matched, mismatch, blocked or unknown with evidence. API-created
state cannot establish native gestures. No implementation acceptance is claimed
by this research contract.

Inspect geometry separately: base volume507*pi, joined volume1229*pi mm3,
valid closed one-solid stock; two bodies before Combine; rigid cutter volume and
radius unchanged by Move; one valid through-bored result. For a fully contained
10-degree bore through11 mm stock, analytic removed volume is
pi*3.1^2*11/cos(10 degrees); use only after checking cutter extent and containment.

Use disposable real previews; Cancel preserves exact authored state. Add one
upstream Join-depth edit, one Move edit, undo/redo, and exact save/reopen check.
Check supported sketch follows cap depth and downstream Combine recomputes.
Inspect actual app captures for blue selected body, readable panel fields,
rotation ring/translation arrows, and red Combine cutter. Source release/scale
is unknown and screenshot crops differ; no pixel-equality claim is supported.
Independent review follows `docs/compatibility.md`. STEP/STL and independent
readback are useful delivery extensions, not source-numbered actions.
