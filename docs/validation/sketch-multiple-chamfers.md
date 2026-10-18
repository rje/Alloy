# SKT-112: multiple Equal Distance sketch Chamfers

Accepted 2026-10-18 for adjacent corners of one authored line sketch. The
dated public Chamfer-family source record is outside the repository at
`../Alloy-reference-records/knowledge/specs/skt-096-sketch-chamfer-2026-10-18.md`.
Multiple-corner repeat behavior was not established by that public evidence;
the acceptance here describes Alloy's policy, not a reference-app parity claim.

Each retained corner has its own source-line pair, generated diagonal/contact
IDs and equal setback. Both cuts derive from the unchanged authored source
lines. On a shared side, they trim opposite endpoints; a duplicate corner or
overlapping setbacks refuse without mutating the draft. The Sketch Palette
offers a retained-corner chooser so either distance can be edited separately.

Evidence from the Release `build-preview-release` app, binary SHA-256
`1d0c4e40626ed95742fcff5eac27d68ffb78191d6236dbd969ee6bd6c891187f`:

- Focused core and native Qt input tests passed 2/2 in 9.22 s. Native input made a second
  adjacent-corner preview and commit, chose and edited each retained corner,
  resized the window, and checked original dimension visibility and hardware
  GPU state. Captures and metadata are at
  `../Alloy-reference-records/validation/2026-10-18-multiple-sketch-chamfer/native/`.
  The focused core case also refused individually valid setbacks whose sum
  exceeds a shared side, source-width collapse, and a duplicate reversed pair.
- Independent [real-app runner](../../tests/sketch_multiple_chamfer_api.py)
  passed 33 local-control commands. It created 10 and 8 mm setbacks at adjacent
  top corners of a centered 150×75 mm rectangle, then independently edited
  them to 12 and 9 mm while changing source dimensions to 160×80 mm. Chamfer
  group/diagonal/contact IDs and body ID stayed stable. The final closed
  profile area was 12,687.5 mm²; its 5 mm solid volume was 63,437.5 mm³.
  An overlapping 155 mm edit refused transactionally, and Undo/Redo and native
  save/reopen reproduced the authored document and evaluated body. Model,
  captures and transcript are at
  `../Alloy-reference-records/validation/skt112-multiple-chamfer-candidate1/`.
- Actual captures were inspected: both diagonal boundaries and both pairs of
  setback labels are visible in the shaded region; the two-corner chooser and
  retained source width/height are readable. This is native GPU presentation,
  not an independent reference-app visual match.

Two Distance and Distance And Angle modes remain SKT-110/#534 and
SKT-111/#535. Direct canvas drag/dimension editing remains SKT-113/#537.
Physical hand input and Windows/Linux runtime are not yet qualified.
