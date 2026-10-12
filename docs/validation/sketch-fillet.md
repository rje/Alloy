# Paired sketch fillet checkpoint

2026-10-12: implementation and bounded geometry/API evidence are available.
SKT-019 remains open because native qualification exposed a Qt accessibility
crash during a later preview/editor synchronization check.

Two selected connected normal-line corners are trimmed into exact analytic arcs
with shared endpoint contacts, retained tangent relations, one radius dimension
and an Equal relation. Surviving line IDs/directions remain. The obsolete500-mm
length dimension is removed and reported; unrelated retired-point references
reject atomically. Preview/Cancel preserve authored state; acceptance is one draft
history entry. The45-degree and100-mm dimensions survive. Radius edits move both
fillets. Arc tangent solving uses radius/contact perpendicularity plus independent
signed-side residual checks.

Public reference: tutorial-illustrated-bracket-a2. Corner pairs follow the lesson
wording and six-edge topology; the small second corner is not independently
legible in the source image. Geometry and visual claims remain bounded accordingly.

## Evidence and unresolved regression

- `build-tutorial/evidence/fillet-core-final.txt`: focused core case passes actual
  outline/end circles/two fillets, radius100→90 and rejection preservation.
- `build-tutorial/evidence/sketch-fillet-ledger/report.json`: actual app workflow
  passes pointer corner picks, preview/cancel, explicit dimension fallout, contact
  identities, shared radius edit, exact undo/redo and save/reopen.
- `fillet-native-{setup,preview,created}.json/.png`: API-prepared outline/end circles;
  native Fillet button, two corner clicks, typing100 and Return creates twoR100 arcs.
  Astra accepted the readable preview, retained dimensions/contact glyphs and
  expected one remaining DOF after removing500. This used the pre-editor-sync binary.
- Astra found API preview100 left editor10. The synchronization source fix is built,
  but postbuild bulkAPI preparation followed by first accessibility observation
  crashes. Matching PID51719 report134028 and debuggerPID52946 both identify
  `QMacAccessibilityElement accessibilitySelectedChildren+184`. FND-015 reopened.

The official proposed Qt patch765434 identifies parent-managed placeholder elements
incorrectly deleting their table accessibility interface. An isolated backport is
being qualified; no patched-runtime acceptance yet. Accessibility stays enabled.
Neither these results nor future fillet closure establishes mixed-circle region
extraction, multi-profile Press Pull or the complete second tutorial activity.
