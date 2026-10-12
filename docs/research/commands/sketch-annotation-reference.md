# Sketch dimensions and constraint presentation

Research and source review: 2026-10-11. This specifies the next straight-line and
rectangle interaction slice; it does not claim implementation or exact parity.
Baseline: [accepted constraints slice](../../validation/sketch-constraints.md),
commit `d018434`. Source inspection of `ReferenceView::decorateSketch` agrees with
that capture: functioning measurements currently appear as detached horizontal
labels, without dimension lines, arrows, placement or canvas editing.

Original URLs, retrieved images, hashes and animation frame locators are in
`../Alloy-reference-records/sketch-annotation-sources.json`, outside the repository.
Earlier community sources resolve in `sketch-controls-sources.json` there. Public
reference images are research material, never application assets. Only public
articles, author transcripts and published image/animation frames were inspected;
no reference application was operated. The supplied image is one additional
source, not the sole specification.

## Evidence and limits

| Source ID | Evidence actually examined | Relevant finding |
| --- | --- | --- |
| `annotation-create` | Official current how-to, Create/Edit/Convert/Tips | Line or two-point dimensions; Enter accepts a value; double-click edits; driving/driven conversion and pre-placement options. Its written steps put numeric entry before placement. |
| `community-sketch-002` | Kevin Kennedy, original dimension article and author transcript, Tutorial Transcript and Driving vs Driven | D, geometry selection, mouse placement, numeric entry, Enter; repeat until Esc. Double-click changes a value; dragging changes annotation position. Driven measurements use parentheses. Publication/version unknown; author acknowledges older UI. |
| `annotation-orientation` | Official article, 2026-06-23, sections 2 and 4 | Pointer location chooses horizontal, vertical or aligned measurement; explicit contextual orientation locks it. Driven mode is available before placement and does not constrain geometry. |
| `annotation-animation` | Official public GIF, sampled frames at 0, 0.78, 1.48, 2.02, 3.43, 3.99, 5.14, 5.69, 6.93, 7.48, 8.49, 9.98 and 10.66 seconds | Preview at 1.48 s, focused numeric editor at 2.02 s, then accepted geometry/annotation at 9.98 s. Black arrows/witness lines, horizontal and rotated vertical text, compact gray relation badges. Input includes units; normal labels omit them. Some hovered/referenced dimensions become blue. |
| `community-sketch-001` | Kevin Kennedy, rectangle author transcript, 2018-12-24 | Opposite corners, automatic H/V constraints, inline width/height and Tab switching. Current rectangle H/V behavior is already qualified in the linked receipt. |
| `community-sketch-003` | Reid Johnson/VDCI, original author transcript, 2024-10-11, narrative after Key Insights | Inference cues, palette visibility controls, profile fill and sketch tools. Instructor choices are not defaults. Linked video was not played. |
| `community-sketch-004` | Kevin Kennedy, original constraint article, 2018-09-29, Coincident/H/V/Sketch Constraint Tips | Compatible-target highlighting while constraining, relation glyphs after applying constraints, contextual relevant constraints and inferred H/V while drawing. |
| `annotation-colors`, `annotation-constrained` | Official sketch overview/type table, fully-defined article and paired illustration | Unconstrained curves blue, constrained curves black, fixed geometry green; per-curve status exists. Closed profiles are shaded. Palette independently hides points, dimensions, constraints and profiles. |
| `annotation-precision`, `annotation-units` | Official Design precision support article, 2026-09-30; public units guide | Display precision/trailing zeros are configurable; numeric input can include explicit units. Display formatting is separate from stored values. Drawing-workspace settings are not sketch evidence. |
| `annotation-user-001` | User-provided 522 × 507 crop | Black dimension graphics, rotated vertical text, radius leader, cyan curves, blue profile fill and gray relation glyphs. URL, date, version, platform and units are unknown. |

The GIF times are source frame-duration offsets actually inspected, not timestamps
invented for an unplayed video. Its tan profile fill differs from the blue fill
in the other illustrations. Exact theme, fonts, platform scale and release cannot
be recovered from these crops. Do not turn any sampled RGB value into a universal
default. Community material plus the animation support placement-before-entry;
the current written how-to describes entry-before-placement. Preserve both entry
opportunities where practical instead of asserting one exclusive sequence.

## Immediate implementation contract

The following is an Alloy design recommendation grounded in that evidence;
specific state names, storage and error policy are our own architecture.

1. **Enter and select.** D or Sketch Dimension enters a visible dimension mode.
   Accept a preselected line or two points; otherwise collect them on the canvas.
   Preview/highlight only compatible targets. Do not silently substitute stale
   selected points for an explicitly addressed line. Preserve the palette as an
   accessible alternative, but do not require visiting it for an ordinary width.
2. **Place.** After targets are known, pointer movement previews the complete
   annotation. For a diagonal pair, choose horizontal/vertical/aligned by pointer
   region, with an explicit orientation override. A horizontal rectangle edge
   gives width and a vertical edge height. The chosen orientation defines the
   solver equation, not merely the text angle. Click fixes placement. Expose this
   preview and placement through the same local commands as physical input.
3. **Enter a value.** Open a small focused numeric editor adjacent to the label,
   selected current value, document units clear. Enter accepts a successful solve;
   keep the tool ready for another dimension. Escape abandons the unfinished
   dimension or value edit before it exits sketch editing. Support explicit mm/in
   entry through the shared parser; reject unsupported expressions honestly.
   Value entry during placement can be supported without a second modal dialog.
4. **Edit and move.** Double-click a driving label opens that editor. Dragging its
   annotation changes placement only, never its measurement or geometry. Preserve
   placement through solve, pan/zoom, Finish and reopen. Driven labels show
   parentheses and read-only measured values; offer explicit driving/driven choice.
   A rejected driving solve preserves the last good draft and offers a deliberate
   driven alternative, rather than silently changing constraint meaning.
5. **Preserve transactions.** Preview, hover and cancelled edits do not add an
   authored dimension. Commit target IDs, kind, value and placement atomically
   into the existing sketch draft; Finish remains one document transaction.
   Label movement must not create a separate undo authority. Resolve selection,
   pending editor and placement state on Cancel, Finish, file changes and session
   invalidation. Retain the prior valid model if any command fails.

## Annotation appearance

For the supported linear family, replace detached text with one presentation:
two witness lines anchored to the selected points, an offset dimension line,
opposed arrowheads and a readable label alongside or interrupting that line.
Use black/dark neutral graphics on the light canvas. Horizontal text follows a
horizontal measurement; vertical text rotates with the vertical line; aligned
text follows the measured direction and is kept readable rather than upside down.
Leave a small screen-space gap from geometry and sufficient arrow/text clearance.
These spacing and collision rules are implementation choices, not measured
reference constants. Small spans need outside arrows/text instead of overlap.

Use an explicit plain UI font, stable logical-pixel text/arrow sizes and a common
unit/precision formatter. The observed normal labels omit unit suffixes while
the editor displays them; follow that compact convention with document units
visible, without rounding solver values. Driven values retain parentheses.
Dimension selection must visibly distinguish hover, selected and ordinary state;
the exact reference selection palette remains unqualified. Hover must not select
or change the sketch. Track the dimension ID for its label, line and hit region.

Render independently authored glyphs for the relations actually stored: H/V,
coincident, parallel, perpendicular and fixed. Compact muted glyphs sit near
their associated geometry with meaningful tooltip/selection identity; they are
not UUID text or decorative marks implying nonexistent constraints. Distinguish
an inference preview from an accepted relation. Show Constraints and Show
Dimensions hide presentation without deleting intent. General automatic snapping,
inference suppression keys and exact glyph-hover behavior need their own bounded
follow-up; only rectangle H/V inference is presently qualified.

Per-entity constrained colors require real per-entity solver diagnostics; overall
zero DOF must not be presented as such diagnostics. Profile shading similarly
requires a recognized valid closed region. Neither should be faked by filling an
arbitrary bounding rectangle. Keep these related gaps visible while completing
the useful line/rectangle dimension workflow.

## Rendering boundary

The present GPU backend can represent lines, arrows, text and interactive
annotations. The existing plain text is a presentation choice, not proof that
OCCT cannot draw dimensions. Public pinned foundation APIs include
[PrsDim_LengthDimension](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/PrsDim/PrsDim_LengthDimension.hxx),
[dimension aspects](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/Prs3d/Prs3d_DimensionAspect.hxx)
and [AIS_TextLabel](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/AIS/AIS_TextLabel.hxx).
All retrieved 2026-10-11; technical integration is reviewed separately. The
unversioned online reference now describes a newer OCCT release.

Keep presentation inputs independent of that backend: stable dimension/relation
IDs, sketch plane, target IDs, measurement kind, driving flag, canonical value,
placement in sketch coordinates and display style. Derive line/arrow/text geometry
and logical-pixel hit regions from these plus the camera. Persist only authored
placement and preferences needed to restore it, never AIS handles or cached pixel
positions. This leaves the accepted RHI migration possible without rewriting
document or solver meaning. A Qt editor overlay must also be included in real
captures; native child-window compositing needs actual qualification.

## Bounded acceptance and follow-ups

Use one anchored rectangle plus one oblique line/point pair. Through the real
shared commands, create width/height with distinct placements, edit a driving
value, add a driven diagonal, move a label, cancel an unfinished dimension and
reject a contradiction. Verify coordinates, unchanged geometry on label movement,
stable IDs, draft preservation, Finish/undo and save/reopen placement. Inspect
horizontal, vertical and aligned annotation captures at comparable canvas scale;
check arrows, witness attachment, text orientation and readable spacing.

Use native input only for the missing physical contract: D, select, move/click,
type/Enter, double-click/edit, drag annotation and Escape. One pan/zoom or Retina
resize check should show annotations and their hit regions remain attached.
Keep this visual/input evidence separate from solver and geometry correctness;
do not repeat unrelated workflows merely to refresh screenshots.

| Gap | Existing work item | Boundary |
| --- | --- | --- |
| Canvas placement, inline entry/editing, annotation graphics and movement | [SKT-005 #32](https://github.com/rje/Alloy/issues/32) | Immediate line/two-point/rectangle slice; not all sketch dimensions. |
| Glyphs, visibility, preselection and trustworthy per-entity status | [SKT-008 #35](https://github.com/rje/Alloy/issues/35) | Show existing relations first; broader inference and diagnostics remain explicit. |
| Profile fill and usable closed-region selection | Existing profile/modeling work | Must follow real region validation, not cosmetic fill. |
| Radius/diameter leaders, angular/parallel-line distance, tangent choices, expressions | Later sketch breadth | Observed/documented, but outside this straight-line slice and current solver contract. |

This record is research and an actionable proposal. No app run, build or test was
performed for it, and no future feature is accepted by writing the specification.
