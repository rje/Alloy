# Sketch annotation rendering on the current GPU backend

Research date: 2026-10-11. Status: source-reviewed design; the subsequent bounded
candidate is qualified on macOS in [the annotation/profile receipt](../validation/sketch-annotations.md). Scope: the existing line/rectangle linear dimension family,
stored relation glyphs and the connection to valid closed-profile display.
The public workflow and screenshot specification is
[sketch-annotation-reference.md](commands/sketch-annotation-reference.md).
This document studies the permitted open source foundations, not a reference
application. Library URLs may remain in the repository.

## Current boundary and visible gap

At the reviewed `d018434` baseline, `src/ReferenceView.cpp` renders real hardware
OCCT geometry. `decorateSketch` adds point markers, selected-edge highlights and
an `AIS_TextLabel` for each dimension. The label has a fixed 3 mm offset, default
orientation, a units suffix and no dimension line, witnesses, arrows or dimension
selection identity. Every decoration is deactivated for native picking.
`pickSketch` separately finds authored points and lines in projected logical
pixels. Solver values are already available by stable dimension ID.

At that baseline, `src/SketchGeometry.cpp` evaluates lines into an edge compound; it does
not evaluate closed regions. Whole-sketch DOF controls wire color, rather than
per-curve freedom. These are real implementation gaps. Adding text styling alone
does not satisfy the public annotation specification, and decorative fill cannot
establish that a region is usable for modeling.

The applicable foundation is the installed OCCT **7.9.3** headers under
`/opt/homebrew/include/opencascade/`, checked against upstream tag `V7_9_3`.
Unversioned online reference pages describe a newer release. Qt documentation
below is pinned to the installed 6.11 family. All linked sources were retrieved
2026-10-11. No build, app operation or native rendering experiment was performed
for this research.

## Stock dimension capability and important limits

| Requirement | Pinned API and source | Integration implication |
| --- | --- | --- |
| Length geometry in a sketch plane | [`PrsDim_LengthDimension.hxx`](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/PrsDim/PrsDim_LengthDimension.hxx): constructor with two `gp_Pnt` and a `gp_Pln`; `SetMeasuredGeometry`, `SetDirection` | Explicitly use the canonical sketch frame. Coincident endpoints do not produce a valid length presentation. |
| Positive H/V projection | [`PrsDim_LengthDimension.cxx`](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/PrsDim/PrsDim_LengthDimension.cxx): `ComputeValue`, `ComputeFlyoutLinePoints` | A custom direction projects the span and changes the main-line endpoints. It does not make every base-class layout helper direction-aware. |
| Lines, arrows, labels and placement | [`PrsDim_Dimension.hxx`](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/PrsDim/PrsDim_Dimension.hxx): `SetDimensionAspect`, `SetFlyout`, `SetTextPosition`, `IsValid`, `SetCustomValue` | Use accepted solver values for label content. Placement setters can reset or adjust other layout properties; recreating a disposable presentation is simpler than retaining its mutable state as document intent. |
| Appearance | [`Prs3d_DimensionAspect.hxx`](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/Prs3d/Prs3d_DimensionAspect.hxx): common color, line/text/arrow aspects, text alignment, arrow orientation, extension/tail sizes, units and numeric formatting | Configure dark neutral lines and arrows explicitly. Units visibility and precision are display policy, separate from stored values. |
| Default behavior | [`Prs3d_DimensionAspect.cxx`](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/Prs3d/Prs3d_DimensionAspect.cxx) | Defaults use green graphics, 2D text and automatic fit. They are not the desired product appearance. |
| Arrow size | [`Prs3d_ArrowAspect.hxx`](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/Prs3d/Prs3d_ArrowAspect.hxx): `SetLength`, `SetAngle`, `SetZoomable` | Prefer world lengths derived from a logical-pixel target for this slice; that keeps stock arrow geometry and its hit geometry consistent. Qualify size at Retina scale. |

There are three material limitations in the pinned implementation:

- The default 2D text path ignores the dimension-line direction when constructing
  the text primitive. The 3D path creates oriented font BRep, while 2D text uses a
  billboard. `drawText` is **not virtual**, so a small text override is unavailable.
- With `SetDirection`, the main line is projected correctly, but
  `GetTextPositionForLinear`, `FitTextAlignmentForLinear` and
  `AdjustParametersForLinear` still work from the original point-to-point vector.
  Consequently, diagonal targets with an H/V dimension must not assume that
  `GetTextPosition` or arbitrary fixed-text placement matches the drawn line.
- Native 2D text selection is a sensitive circle around the anchor, not a label
  rectangle. The 3D-text branch creates rectangular text selection geometry.

These are source observations in
[`PrsDim_Dimension.cxx`](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/PrsDim/PrsDim_Dimension.cxx),
particularly `drawText`, `DrawLinearDimension`, `ComputeSelection` and the linear
layout helpers. Their practical effects still require one bounded native check.

## Smallest recommended implementation

Keep one small, disposable layout calculation near `ReferenceView`; it needs
stable IDs, the canonical plane/frame, two accepted target points, measurement
kind, accepted measured value and authored placement. It should return dimension
line endpoints, witness endpoints, label position/direction and hit regions. This
is presentation math for the supported workflow, not a new general CAD framework.

For an aligned measurement, the dimension direction is the normalized target
span. For horizontal/vertical it is the sketch u/v axis. Let `d` be this unit
direction and `f` its in-plane perpendicular. A placed point `c` defines the
offset line. Project each target `a`, `b` onto that line:

```
A = a + f * dot(c - a, f)
B = b + f * dot(c - b, f)
```

Construct `PrsDim_LengthDimension(A, B, sketchPlane)` with zero flyout. It now sees
ordinary collinear presentation endpoints, so stock arrow fit and label layout
do not depend on custom-direction special cases. Add two GPU witness segments
from the actual targets to `A` and `B`, with a small logical-pixel geometry gap
and overhang converted through the current camera. OCCT edge/AIS wire presentation
already used by `ReferenceView` suffices for those two segments. These endpoints
are display data; authored targets remain the original stable point IDs.

Use a per-dimension dark aspect, planar arrows and explicit font/height. Pass the
formatted accepted solver value through the string overload of `SetCustomValue`;
disable automatic units suffixes and retain parentheses for driven values. This
uses the same formatter as the numeric editor and avoids a second unit authority.
Labels must never update the solver from an independently computed OCCT value.

For the first plane-normal sketch slice, use stock 2D text with an explicit angle
from the projected dimension direction. Normalize it to a readable orientation.
`Prs3d_TextAspect::SetAngle` reaches
[`Graphic3d_Aspects::SetTextAngle`](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/Graphic3d/Graphic3d_Aspects.hxx),
which uses degrees; the billboard path in
[`OpenGl_Text.cxx`](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/OpenGl/OpenGl_Text.cxx)
applies that rotation. Recompute angle and pixel-to-model size when the camera or
display scale changes. The screen-space sign, readable vertical direction and
font size must be checked in the real app rather than assumed from source alone.

An alternative is `MakeText3d(true)` plus `MakeTextShaded(true)`: stock oriented
font BRep and rectangle picking. Its height is in model units and must be adapted
to zoom for consistent apparent size, regenerating presentation. This is viable
for a few dimensions, but its font geometry and zoom cost should not be adopted
as a permanent text system without measurement. Neither option changes the
hardware requirement or the accepted RHI destination.

The subsequent candidate uses a narrower hybrid: stock `PrsDim` supplies main
lines/arrows with its label empty, while an explicitly rotated `AIS_TextLabel`
occupies the exact authored anchor. The dimension line is derived from that
anchor with perpendicular logical-pixel clearance, keeping text alongside it.
The first GPU subtitle attempt did not reliably mask the main line, so the
candidate now uses real geometric separation. This avoids stock centering of in-span labels when users drag them. The
label's angle, font and hit bounds share the portable layout. Complete graphics,
native legibility and clearance still need the candidate-specific check; this
does not claim that a detached text label alone is an implemented dimension.

The first candidate capture exposed an angle-unit mistake in that narrower path.
Unlike `Graphic3d_Aspects::SetTextAngle`,
[`AIS_TextLabel::SetAngle`](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/AIS/AIS_TextLabel.cxx)
accepts radians and converts to aspect degrees internally. The correction passes
layout degrees through an explicit radians conversion. This distinction must be
retained when changing either presentation path. The same capture also required
neutral sRGB colors and DPR-scaled line widths for readable dark graphics.

Persist optional placement in sketch coordinates on the dimension record through
the existing draft commands. Old records get a derived default. Cache native
objects only in `ReferenceView`. Pointer preview and cancelled input add no intent;
accepted value, kind, target IDs and placement enter the draft atomically. Dragging
a label changes only placement. A rejected solve preserves the last good draft.
The companion workflow spec owns D, placement, inline Enter, double-click and
Escape sequencing.

### Picking and native controls

Dimension selection can use `Activate(object, PrsDim_DimensionSelectionMode_Text)`
and the Line mode; map `DetectedInteractive` back to stable dimension ID and
`PrsDim_DimensionOwner::SelectionMode()` to the annotation part. See
[`AIS_InteractiveContext.hxx`](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/AIS/AIS_InteractiveContext.hxx)
and [`PrsDim_DimensionOwner.hxx`](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/PrsDim/PrsDim_DimensionOwner.hxx).
Do not globally activate all scene objects. Preserve the current fractional
logical/native conversion and authored point/line selection priority.

For 2D labels, supplement the anchor circle with a logical-pixel rectangle from
the displayed text/font metrics and rotation. Expose that rectangle and stable
ID in annotation inspection so native input and API picking use the same targets.
OCCT font resolution/fallback may differ from Qt metrics; the bounded capture and
click check must establish that the full label is actually usable. Prefer this
small layout-owned picking to reconstructing identity from floating coordinates.

Keep all lines, arrows, text and hover/selection graphics in the GPU scene so
`ReferenceView::framebuffer` captures them. A small native Qt value editor is a
control, but its layering, focus and inclusion in whole-window captures need
qualification. [Qt 6.11 QWidget documentation](https://doc.qt.io/qt-6.11/qwidget.html)
describes native-widget behavior and separate window-container stacking/capture
limits. Alloy uses a native QWidget surface, not `createWindowContainer`; those
limits are a reason to verify composition, not proof that its overlay fails.
Do not substitute a QPainter-rendered CAD scene or assume transparent child
widgets will behave identically on three platforms.

## Constraint glyphs and point visibility

The existing point code uses `Aspect_TOM_POINT` for ordinary endpoints. Its scale
does not enlarge the dot, so increasing the current numeric size cannot make those
points readable. Use an explicit zoom-independent circle/dot marker where the
public spec calls for one. For H/V, coincidence, parallel, perpendicular and fixed,
render independently authored compact icons for relations actually stored, with
constraint ID, target IDs, anchor and logical-pixel bounds. Keep inference previews
separate from accepted glyphs. Anchor single-line glyphs near the midpoint and
point relations near their point; simple bounded separation suffices initially.

The existing `AIS_Point` can use a
[`Prs3d_PointAspect`](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/Prs3d/Prs3d_PointAspect.hxx)
with a custom
[`Graphic3d_AspectMarker3d`](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/Graphic3d/Graphic3d_AspectMarker3d.hxx)
bitmap or image. This provides a GPU marker route for original icons without
building a custom presentation class. Use camera-derived world anchors for the
screen-pixel offset, and own screen-space hit bounds for glyph hover/selection.
Visibility preferences hide graphics, not constraints. Whole-sketch zero DOF
cannot supply per-entity constrained colors; retain that diagnostic gap explicitly.

## Profile fill requires evaluated regions

Keep closed-profile work with the geometry/document owner. A first valid simple
line loop needs ordered authored connectivity, closure, nonzero area and rejection
of intersections/branching. Do not hide a gap by merging unrelated point IDs or
infer a profile from a bounding rectangle. General intersecting networks, nested
holes and persistent region identity need the later profile contract.

For a qualified simple loop, use
[`BRepBuilderAPI_MakeWire`](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/BRepBuilderAPI/BRepBuilderAPI_MakeWire.hxx)
then [`BRepBuilderAPI_MakeFace(gp_Pln, wire, true)`](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/BRepBuilderAPI/BRepBuilderAPI_MakeFace.hxx).
Check construction success and
[`BRepCheck_Analyzer::IsValid`](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/BRepCheck/BRepCheck_Analyzer.hxx)
before exposing it as a usable region. Builders alone do not establish the product's
entire closure/intersection policy. Render the accepted face as a muted translucent
`AIS_Shaded` object behind its edge/annotation presentation; add profile selection
only when a stable evaluated region ID is available.

The same face can be meshed with
[`BRepMesh_IncrementalMesh`](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/BRepMesh/BRepMesh_IncrementalMesh.hxx)
and read through [`BRep_Tool::Triangulation`](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/BRep/BRep_Tool.hxx).
Apply the returned face location and face orientation when producing renderer
vertices/indices. Tessellation is disposable display data. A filled valid face is
the useful bridge to extrusion, but profile fill alone does not implement extrusion.

## RHI migration and bounded qualification

Follow [decision 003](../decisions/003-renderer-direction.md): keep OCCT as kernel
while an established RHI replaces AIS/V3d presentation. Stable IDs, sketch-plane
coordinates, accepted measured values, placement and original glyph assets are
portable. Derived witness/line segments, arrow geometry, text/font/style and hit
bounds can become the RHI draw inputs. AIS handles, native selection owners and
GPU buffers remain backend caches, outside files, undo and persistent references.
Do not build an abstract renderer now just to draw three dimensions.

Use one anchored rectangle and one oblique pair. Check width/height change from
40 × 25 to 50 × 25, driven diagonal, distinct placed/dragged labels and unchanged
geometry on label movement. Inspect horizontal, vertical and aligned graphics,
arrow fit for one small span, real target attachment and full-label picking.
Include one diagonal H/V placement specifically to catch the stock custom-direction
layout limitation. One pan/zoom and Retina resize/capture verifies attachment,
readability and hit bounds alongside the existing hardware diagnostics. Save/reopen
and Cancel cover authored placement. Separate output checks, native interaction,
visual comparison and GPU evidence; do not rerun unchanged unrelated workflows.

The subsequent macOS candidate qualifies readable H/V/aligned graphics, beside-line
clearance, actual filled faces, inline editing and annotation dragging for the
recorded rectangle. Exact reference font/scale, cross-platform editor composition,
general collision handling and Windows/Linux runtime remain unqualified.
The source review and bounded capture do not establish whole-product visual parity.
