# Sketch palette summary layout

FND-022/#211, 2026-10-13. The multi-face widget capture exposed a summary
allocated only15 logical pixels despite a45-pixel minimum/height-for-width hint.
Its plane, edge count, instruction and DOF lines were clipped below Name; an
unexplained gap appeared before selection guidance. Baseline evidence is in the
multi-face candidate's `build-multi-face/evidence/widget/plate40-rectangle.json`
(`validationLayout`) and matching PNG.

The scroll content layout now uses `QLayout::SetMinAndMaxSize` so changing child
size hints propagate through its resizable scroll area. No command, geometry,
selection or model behavior changes. The existing palette remains312 logical
pixels wide; narrower-window captures do not claim user-resizable palette widths.

The main application build passes. One saved side-face sketch was reopened and
inspected through the actual application at requested1400×900 and1100×900 window
sizes. The Select summary, Center Rectangle instruction and longer wrapped Offset
instruction are fully visible, with normal spacing below Name. Selection guidance
and the existing controls remain legible; Finish/Cancel stay outside the scroll
area and visible. Cancel preserves exact authored data. The private apps exit0.

Captures/state/transcripts are in `build-tutorial-polish/evidence/palette-summary/`
and `palette-summary-long/`. These are real GPU application captures and shared
API edit/tool/Cancel checks, not physical mouse evidence. The existing scroll
container and control actions are unchanged; no new general UI test suite or
complete tutorial replay was needed. Toolbar caption truncation at narrow widths
is outside this correction. Exact reference styling is not claimed.
