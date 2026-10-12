# Illustrated bracket workflow

Recorded 2026-10-12. Application candidate is the integrated profile-Cut code at
`84b89c3`; workflow case and receipt-schema correction are `44a1104` and `18ed412`.
The candidate uses the coherent private Qt runtime described in
[the accessibility recipe](../dependencies/qt-cocoa-accessibility.md).
Actual GPU diagnostics report Apple M4 hardware rendering.

## Executed results

`python3 tests/workflows/run.py --case tutorial_bracket_api --app build-tutorial-api/alloy.app/Contents/MacOS/alloy --evidence build-tutorial-api/evidence-final`
passed. The report and individual receipt contain the command transcript,
13 source-stage captures, geometry checks and exact undo/redo/save/reopen checks.
Astra independently accepted the source order, numerical checks and representative
images. This case starts with an empty real application document; it does not
load prebuilt modeled geometry. It has `covers: []` because broader issue and
physical/visual acceptance remains separate.

A separate Tutorial Candidate was operated natively from a new design tab.
Its checkpoints are in `build-full-tutorial/evidence/`; the final authored model
is `native-full-bracket.alloy`. API calls inspected/projected geometry, navigated
the camera, captured images and saved checkpoints. The two selection substitutions
below are explicitly additional API modeling input.

| Source activity | Observed result | Native boundary |
| --- | --- | --- |
| A1 outline and constraints | Six connected origin-based lines; three Perpendicular, two Horizontal, two Equal; placed 500/45/100 dimensions; DOF 0 | Native line drawing, constraint picks and numeric dimensions. An extra Finish/save/reopen occurred before A2; not an uninterrupted exact sequence. |
| A2 construction and end circles | Both cap IDs retained as construction; midpoint/tangent circles of radius 50 | Two-cap selection supplied by API because CUA lacks held Shift-click. Native context-menu conversion and both circle placements passed. |
| A2 paired fillets | Two tangent radius 100 arcs; only affected 500 dimension removed; 45 and100 preserved; DOF 1 | Native joint selection and common radius entry. Longitudinal DOF after fillet is intentional; no replacement length was forced. |
| A2 solid | Three selected regions, distance 50, one valid closed solid | Native Q, three ordinary profile clicks, depth field and OK. |
| A2 supported guides | Create Sketch then actual top-face pick; two retained curve-center anchors; connected guides and two external Parallel relations; DOF 0 | Native face/line/constraint picks. Two-guide selection supplied by API before native context conversion. |
| A3 circles | Endpoint/midpoint/bend/midpoint/endpoint centers; initial inline diameter 50; four Equal pairs; all radii 25 and DOF 0 | Native C, snaps, inline typing and repeated Equal pairs. |
| A3 Cut | Five source profiles; arrow dragged to -200; Cut; one retained body with five holes | Native Q, ordinary profile picks, arrow drag, operation dropdown and OK. Physical Cmd/Ctrl toggle remains unverified. |

The native final solid has volume 4694718.448385876mm³ and removes
490873.8521234039mm³, agreeing with five radius 25 holes through 50 mm stock.
The API case independently reconstructs post-fillet stock area and verifies the
same result. Its final longitudinal size is evaluated from the solved geometry,
not treated as a uniquely specified tutorial value after removal of 500.
`native-full-storage.json` confirms exact authored restoration through API
undo/redo and save/reopen of the natively created model.

## Remaining acceptance

- Both explicit held-Shift selection gestures and physical platform profile
  toggling remain unverified. Synthetic Qt modifier coverage is separate evidence.
- Native A1 included an extra checkpoint Finish/reopen. This run proves the
  modeling chain with that deviation, not uninterrupted exact source execution.
- Construction color was corrected in `324fb74`: DOF 0 guides are dark dashed,
  selected guides remain orange, and projected references remain purple.
  Astra accepted the actual `build-tutorial-polish/evidence/*guide-color.png`
  captures, including the DOF 1 outline's orange construction strokes. This is
  whole-sketch solver coloring, not a per-entity freedom classification.
  Menu hierarchy, toolbar layout and unmatched reference camera/theme/DPR still
  prevent an exact visual claim.
- `e84703d` corrects the missing support stock when editing the earlier hole
  sketch after Cut. The viewport displays the actual pre-sketch history prefix,
  preserving body visibility, and restores the final result on Cancel or Finish.
  `build-tutorial-polish/evidence/historical-support-probe.json` and the matching
  edit capture passed Astra review: supporting stock visible, exact authored
  restoration, and explicitly hidden stock remains hidden. This focused check
  reused the native final model; the full geometry tutorial was not rerun for
  these display-only changes.
- Qt accessibility row labels can disappear or become stale after edits despite
  correct visible contents; FND-015 remains open. Native coordinate input was
  used where those labels were incomplete.
- The first runner attempt completed the application case but failed aggregating
  string-shaped evidence metadata. `18ed412` corrected the receipt schema; the
  subsequent registered run passed. No application geometry fix was required.

TUT-003 and broader owner issues remain open. No release, tag or CI run was
triggered. Research URLs and source screenshots remain outside the repository.
The external `knowledge/tutorials/illustrated-bracket/step-status-2026-10-12.json`
maps all 49 source subinstructions and transition/selection notes to this run's
API/native evidence and explicit gaps. Its color-pending entry is superseded by
the accepted correction above; it remains a dated audit, not a live test result.
