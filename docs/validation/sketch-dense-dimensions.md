# Dense sketch dimension presentation

SKT-190/#713, 2026-10-20. Independent Sol source/evidence review accepted
candidate `9107e98` for the bounded five-driver fixture. This is a presentation improvement, with no
solver, schema, dimension value or transform-authority change.

The recorded multi-profile sketch previously put five driving dimension labels
on top of one small retained Rotate profile. Visible ordinary dimension layouts
now share one resolved screen-space placement for GPU text, annotation picks,
state inspection and direct-value editor positioning. Colliding rotated text
footprints form local clusters. A cluster gets horizontal callout rows in an
available lane beside its original projected annotations, with thin leaders and
white text backing. Stable projected order and dimension IDs determine the rows.
Noncolliding labels retain their exact original layout. Nothing is hidden to
force a fit; a cluster that cannot fit keeps all labels and reports
`crowdedUnresolved` in its display layout.

Camera changes and native resize recompute the display layout. Raw authored
annotation anchors, dimension lines and witnesses remain unchanged. Selecting a
callout writes no placement. Double-click edits the same dimension through the
existing inline editor beside the resolved label. An ordinary annotation drag
captures the visible grab offset, stays at the pressed display location, and
adopts the deliberate visible target through the existing annotation command.
Retained Rotate source drivers keep their existing annotation-drag refusal and
numeric source-size edit route.

Public evidence R1–R3 documents manual placement and double-click/value editing;
R4–R6 informs the crowding mechanism and placement examples. Automatic lanes,
thresholds, leaders, text backing and ordering are Alloy policy, not a claim of
reference automatic behavior or exact styling. Research and architecture remain
external in `../Alloy-reference-records/knowledge/specs/`:

- `skt190-dense-dimension-visual-2026-10-20.md`
- `skt190-dense-dimension-architecture-2026-10-20.md`

## Focused evidence

Built the application and `sketch_dense_dimensions_controls_test` in the existing
`build-preview-release` Release cache, Qt6.11.2/OCCT7.9.3, with `-j4`.
The focused real-entrypoint Qt fixture passes **3/3, 13.692s**, with 49 shared
local commands plus actual native pointer/key delivery. Initial harness
activation is explicit; no interstep focus or selection repair occurs. Pointer
receivers are checked with `QApplication::widgetAt`.

Evidence directory:
`../Alloy-reference-records/validation/skt190-2026-10-20/native-final/`.

- `dense-five-labels.png`: all five full values remain visible with distinct
  padded hit footprints at55% zoom. Current camera scale279.089mm differs
  from the earlier capture's306.525mm; DPR/window/zoom match, camera scale does
  not. Each actual hit selects the expected stable
  dimension ID. Each double-click opens the focused editable value field;
  Escape returns viewport focus and preserves exact draft/history. Editor origin
  follows the resolved label. `dense-direct-value-editor.png` records typed but
  cancelled input.
- `dense-resized.png`: after pan(+24,−16) and native1320×850 resize, all five
  footprints remain separated and authored data/history remain exact.
- `source-local-mixed-layout.png`:16× cursor magnification from55% gives880%
  overall zoom, camera scale31.7146mm and a useful source-local framing. Three
  labels retain byte-for-byte raw layout; two still need callouts because their
  conservative rotated footprints intersect. A normal zoom does not disable
  genuine collision relief.
- `normal-two-dimensions.png`: an ordinary asymmetric four-Line profile with two
  deliberately authored outside annotations has exact unchanged raw layouts at
  a readable source-local zoom. It does not depend on coincident default anchors.
- `ordinary-label-drag.png`: after adding the remaining three drivers and zooming
  out, an actual displaced-label drag ends within one logical pixel of the
  visible target. Points/entities/constraints stay exact; Undo/Redo restores the
  complete authored before/after states. The retained-owner drag refusal is
  separately checked without a display jump or history change.
- `report.json`: draft/layout/state/transcript records. Presentation-only steps
  preserve the complete draft, all five IDs/values/references, retained owner and
  source/output cache. Finishing preserves complete authored JSON and evaluated
  body summaries; save/reopen preserves authored JSON exactly.
- `before.alloy`, `after.alloy`, `before.stl`, `after.stl`:
  real-app three-profile8mm Extrudes preserve analytic total5792mm³. Independent
  binary-STL readback (`independent-stl.json`) gives identical summaries before
  and after: three watertight consistently wound components,36triangles,
  24vertices,54edges, Euler6 and5792.000032559321mm³. Each component has finite
  unit facet normals aligned with winding.

GPU state records Apple M4, Apple vendor, OpenGL4.1 Metal-90.5, GLSL4.10,
DPR2 and1864×1232 native framebuffer for the932×616 logical viewport. Actual
GPU captures were inspected for dense, resized, ordinary and source-local views.

Run the focused check with absolute evidence paths:

```sh
cmake --build build-preview-release --target sketch_dense_dimensions_controls_test -j4
ALLOY_DENSE_DIMENSION_FIXTURE=/absolute/crowded-source.alloy \
ALLOY_DENSE_DIMENSION_EVIDENCE=/absolute/evidence \
./build-preview-release/sketch_dense_dimensions_controls_test
```

## Limits and validation corrections

Leaders can cross compact source strokes; the active inline editor may cover
neighboring callouts while open. Values remain direct targets outside the
editor. This is a small deterministic lane policy, not a global annotation or
leader-crossing optimizer. It uses conservative rotated AABBs and can relocate
text whose exact glyph outlines would fit. Extremely dense/tiny viewports may
report unresolved clusters without suppressing dimensions. The accepted SKT-190 baseline attempts only four local lanes.
A [bounded margin-stack candidate](sketch-dimension-overflow.md) now extends
exhausted lanes under SKT-191/#714; independent review is pending. Existing chamfer
owner glyphs outside the ordinary dimension collection are unchanged.

Early fixture attempts incorrectly required all five default labels to become
raw at modest zoom, assumed a new ordinary sketch was fitted at readable scale,
or supplied an out-of-contract single zoom factor/project-point request. These
were harness assertions/arguments, not solver changes; final checks use recorded
camera/layout evidence and an explicitly readable normal fixture. The native
run confirms supported ordinary dragging and preserves existing retained-owner
refusal rather than broadening transform metadata authority.

No proprietary application was operated. No CI, release, push or platform
package qualification was performed. Physical trackpad and Windows/Linux native
behavior remain independent release gates. Private validation editors were
closed and user PID72618 was preserved.
