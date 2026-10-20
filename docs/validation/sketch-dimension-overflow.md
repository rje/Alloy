# Sketch dimension margin overflow

SKT-191/#714, 2026-10-20. Independent Sol source/evidence review accepted
candidate `2c21d9f` for the bounded seven-dimension, minimum-window fixture.

When all four local lanes fail, visible ordinary dimensions now share a finite
screen-space row grid at the nearer canvas margin, then the opposite margin.
Every row keeps its full value, stable dimension ID, selected color, direct hit
and double-click editor. Thin leaders connect the original annotation to the
resolved label. The same resolved layouts drive GPU drawing, inspection, picks
and editor placement. The reserved cube/accessory region and other resolved
labels remain obstacles. No authored placement, schema, solver, value or geometry
authority changed. Noncolliding authored layouts and local lanes remain preferred.

This completes the bounded fallback proposed by the external SKT-190 visual and
architecture records. The compact follow-up contract is external in
`../Alloy-reference-records/knowledge/specs/skt191-margin-dimensions-2026-10-20.md`.
R1–R6 provenance remains in the adjacent SKT-190 visual record. Automatic layout
and exact reference styling remain **unknown**, and this margin policy is Alloy's.

## Evidence

The application and focused Qt fixture built in the existing
`build-preview-release` Release cache, Qt6.11.2/OCCT7.9.3, with `-j4`.
Run only the focused `marginOverflow` slot with the same absolute fixture and
evidence environment variables documented in [SKT-190](sketch-dense-dimensions.md).
`QT_SCALE_FACTOR=0.5` on this native DPR2 display produces effective DPR1.
The focused slot passes **3/3 at DPR1 (3.216s)** and **3/3 at DPR2 (5.327s)**,
with37 shared local commands in each run.

Evidence is external in
`../Alloy-reference-records/validation/skt191-2026-10-20/final-dpr1/` and
`final-dpr2/`. `probe.json` records original/resolved layouts and initial GPU
state; `overflow-report.json` records subsequent layouts, native resize state,
exact body summaries and the shared-command transcript.

- The reused five-driver Rotate fixture has two additional independently
  authored numeric dimensions on its ordinary second profile, with coincident
  text anchors creating a separate crowded local cluster. At supported1040×640,
  five source values occupy distinct left-margin rows; the second pair remains
  readable in local callouts. All seven full padded footprints remain inside
  the canvas and outside the reserved controls, and no pair overlaps.
- Each actual `QApplication::widgetAt` receiver is checked. Seven single clicks
  select the expected ID and seven double-clicks open the focused writable
  editor for that ID. The editor uses the expected clamped origin and stays
  inside the canvas. Escape restores viewport focus and exact draft/history.
  The margin-positioned retained source preserves its annotation-drag refusal
  without a display jump or history write. Supported ordinary annotation
  dragging retains the unchanged shared route accepted in SKT-190.
- Pan(+12,+8) keeps all labels readable and reversing pan restores exact layouts.
  Native/API1070×670 resize recomputes readable rows; returning to1040×640 keeps
  IDs, full text and source margin with centers within2 logical pixels. A zero
  pan refresh at the returned native state is exact. A1500×950 source-local
  zoom restores at least three source labels byte-for-byte to authored raw
  layout. Presentation steps preserve the complete draft and draft history.
- Save/reopen preserves complete authored JSON. The three evaluated8mm bodies
  retain exact summaries and analytic total5792mm³. One new downstream STL is
  byte-identical to accepted SKT-190 `native-final/before.stl` (SHA256
  `e1bece0762ee95c9d92bef84702bbaec94626d057327aa16959ec2cf5fce0326`).
  The accepted independent readback therefore applies: three watertight,
  consistently wound components,36triangles,24vertices,54edges, Euler6 and
  volume5792.000032559321mm³. No repeated STL validation matrix was needed.

Actual GPU captures `overflow.png` and `overflow-resized.png` were inspected at
both effective scales. Apple M4 / Apple / OpenGL4.1 Metal-90.5 / GLSL4.10 remains
the hardware path. DPR2 has472×307 logical /944×614 framebuffer initially and
502×337 /1004×674 after resize. Effective DPR1 has472×307 logical /472×308 native
initially and502×337 logical /502×338 native after resize. Returned DPR1 is
472×308 logical/native. The native backing and logical height can differ by
one pixel; reports retain both sizes at every recorded state. This rounding
limits exact resize-return world-anchor equality; readability and subsequent
same-state determinism are checked separately.

## Limits

The console was locked. Tests use explicit initial Qt harness activation and
Qt-delivered input with checked actual hit recipients; these are not strict
physical OS input qualification. No focus or selection repair occurs between
steps. Physical input and Windows/Linux runtime remain separate gates.

This is a finite two-margin policy with no global packing or leader-crossing
optimizer. Leaders can cross compact geometry and one another, but have distinct
label endpoints. The editor temporarily overlays nearby geometry/labels while
open. More labels than both margins can hold remain fully present and report
`crowdedUnresolved`; arbitrary finite-canvas density is not solved.

Earlier probes used a test-only subminimum window, incorrectly required a margin
stack to persist when pan/resize reopened a local lane, or assumed exact native
backing height at DPR1. They are not accepted evidence; final runs use supported
window sizes, require reflow rather than one permanent lane, and record native
rounding separately. No proprietary application, CI, release, push or external
publication was used. Private fixture processes exited; user PID72618 remains.
