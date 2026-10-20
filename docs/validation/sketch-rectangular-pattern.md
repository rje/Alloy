# Retained rectangular sketch-circle Pattern candidate

SKT-097/#502; 2026-10-20. This bounded candidate keeps one ordinary authored
circle and two explicit authored perpendicular construction Lines in one planar
sketch. The external specification and architecture audit are
`../Alloy-reference-records/knowledge/specs/skt097-sketch-patterns-2026-10-20.md`
and `skt097-architecture-2026-10-20.md`. Their R1/R3 records contain permitted
public documentation and provenance retrieved 2026-10-20. No proprietary
application was operated. Detailed reference names and URLs remain external.

## Command and authored contract

Sketch > Create > Rectangular Pattern opens native Objects/Direction 1/Direction 2
identity selectors and per-axis Quantity/Distance fields. These controls invoke
`sketch.patternPreview`, `sketch.patternAccept`, and `sketch.patternCancel`.
An original small grid glyph next to the source circle opens the same editor on
double-click through `sketch.patternEdit`. The Create route also edits an existing
group. Escape cancels the native editor. Preview changes neither the sketch draft
nor history; OK publishes one sketch edit. A later Finish Sketch commits through
the existing document authority. History Undo first cancels a pending preview.

Alloy policies, rather than claimed reference defaults: one group, one circle,
two perpendicular construction directions, Spacing / One Direction, Quantity
**including the source**, initial counts 2 × 3 and distances 14 / 11 mm. Counts
must be integers 2..16, with at most 64 total circles. Spacing is positive
0.001..100000 mm; generated coordinates must stay within ±100000 mm. Touching or
overlapping member circles refuse. Explicit line start-to-end directions define
the signs. Arbitrary seeds, one-axis rows, Extent, Symmetric, canvas manipulators,
suppression and copied constraint semantics remain broader ADV-005 scope.
Circular Full/Angle remains SKT-177/#700 Todo.

`parameters.patternGroups` retains a UUID, source/direction UUIDs, modes, counts,
spacings and `(i,j)` member rows. The source remains index `(0,0)` with its original
circle/center identity. UUIDv5(group, versioned source/index/role) gives each result
circle and center a deterministic identity; spacing/count edits preserve surviving
indices and regrowth restores removed identities. Ordinary circle/point arrays
hold redundant display/profile caches. Every solve validates full ownership,
removes result rows from the PlaneGCS input, solves source/direction intent and
then derives the caches from that solved geometry. Results add no solver freedom;
inspection reports derived ownership and source-dependent freedom. Cache geometry
is recomputed, while altered membership or identity refuses.

Result constraints, driving/driven dimensions, Fix, drag and dependent retained
sources refuse. Delete of the source, directions, result circles or their owned
points refuses atomically with association guidance. Source center/radius and
ordinary direction edits remain authoritative. Generated circles contribute real
analytic profiles. A retained Cut stores an explicit profile set: count growth
does not silently add new holes; reduction removing a referenced profile refuses
during preview before a draft/history edit. The native and API preview share this
full-document dependency check.

## Evidence and limits

External candidate evidence is under
`../Alloy-reference-records/validation/skt097-2026-10-20/`. The accepted build
configuration is `build-preview-release`, Release, private Qt 6.11.2 and installed
OCCT, built with `-j4`. Core fixture `sketch_pattern_test` covers exact centers,
source identity, source-only DOF, derived diagnostics, indexed identity and
regrowth, source radius/center edits, invalid count/spacing/collision/direction,
Delete/drag/Fix/dimension refusals and exact history/save/reopen.

The real-executable fixture `tests/sketch_pattern_api.py` starts from blank,
previews/cancels/accepts an unequal 2 × 3 grid at (8,7), R1.5, then makes a
42 × 40 × 5 asymmetric plate and a six-profile through-Cut. A retained source
edit to R2 and (9,8) recomputes the existing Cut. Exact expected final volume is
`8400 - 6*pi*2^2*5 = 8023.008881569225 mm³`. Independent stdlib STL triangle
readback checks finite aligned facet normals, oriented manifold edge pairs,
one positive-volume connected component and Euler characteristic −10 (six
through-holes). This is separate evidence from OCCT exact geometry and GPU
presentation. Authored save/reopen, draft Undo/Redo and missing downstream-profile
refusal are asserted independently.

The initial native fixture refused before delivering inputs because the nonmodal
editor remained behind its parent in Qt's active-window state. The failed
observation is retained in `native-initial/refused-activation.txt`. A subsequent window-modal attempt also refused native activation. An embedded
probe then exposed a QDialog window-flag promotion and edit selector type mismatch;
explicit widget flags and QString lookups correct those product defects. The production
editor now uses an embedded command dock in the active main window. Its initial
keyboard focus stays in the canvas; the fixture records that state and uses
normal actual-hit pointer steps before typing. The fixture performs only initial
application harness activation; later steps observe actual main-window activation,
focus, hit targets and closure without repair.
Final native input/GPU qualification and independent review are recorded below
when completed. Do not infer physical OS input from Qt-delivered events.

Public documentation matches the active-sketch/Create/Rectangular Pattern naming,
Objects/direction inputs, Quantity/Distance and pattern-glyph edit concept.
Detailed dialog layout, colors, defaults, preview opacity and glyph art are
**unknown** because the public image is an arrangement example rather than a
matched application capture. Controls and glyph art are independently authored.
Physical OS input and packaged Windows/Linux runtime remain release gates.

## Final local candidate receipt

- `sketch_pattern_test`: PASS, `RECTANGULAR_PATTERN_CORE_OK`, including changes
  to both axis counts and both spacings and exact indexed regrowth.
- `native-candidate.log`: 3/3 Qt test functions pass (one complete workflow plus
  initialization/cleanup), 2.617 s. Normal actual-hit Cancel/OK, visible grid-glyph
  double-click, glyph-editor Cancel, typed 14→16 mm spacing, distinct Create/Edit
  Undo/Redo and 1500 × 950→1320 × 880 resize pass. `activation.json` records the
  actual focused source selector, parent activation and glyph hit bounds. There
  is no inter-step test focus/selection repair. These are Qt-delivered component
  inputs, not physical OS input. Escape is implemented but not exercised by this
  bounded fixture. Explicit native sources use readable identity selectors;
  viewport geometry picking/manipulators are not claimed.
- `api-candidate/transcript.json`: the final real-executable 73-call run passes
  preview preservation under generic inspection, Cancel/OK, both count/spacing
  edits, source edits through the retained Cut, missing-profile refusal, history,
  save/reopen and mesh export. `report.json` includes all six independent hole
  centers, 864 wall-vertex samples per hole, exact volume
  8023.008881569229 mm³ and independent STL volume 8023.128483098437 mm³
  (3492 triangles; one closed oriented component, Euler −10).
- Apple M4 / Apple, OpenGL `4.1 Metal - 90.5`, DPR2 and native FBO capture are
  recorded. Actual captures show six analytic profiles, source radius annotation,
  dashed construction directions, the original edit glyph, readable native
  controls and the smooth through-hole plate. Resized framebuffer dimensions
  equal the current viewport dimensions times its recorded scale. Native DPR1
  and packaged Windows/Linux GPU/input remain unqualified.

The earlier failed qualification logs/captures remain external. Product corrections
include preserved pending geometry during generic inspection, pending-grid Look At,
explicit embedded-widget flags, attachment to the actual canvas dock host,
QString selector restoration, and one undo item per accepted edit. Neither a
failed earlier capture nor a matching arrangement establishes exact visual parity.
All private test/editor processes were closed. Independent compatibility review
remains pending with the integrating agent; no CI or release was run.
