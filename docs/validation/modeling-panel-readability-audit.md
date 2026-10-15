# Modeling panel readability audit

Validation was captured on 2026-10-14 from the local Qt application through
Alloy's API and widget inspection. This is presentation evidence at the tested
window sizes; it does not establish pointer-input parity or broader workflow
coverage.

## Fillet and Combine

The audit opened [`gauge-block.alloy`](../../examples/gauge-block.alloy), then
entered Fillet with the block as target and no selected edges. At 1360×850 and
1040×640, the captures show the empty-edge message, disabled OK, and visible
Cancel. The API cancelled the command; no Cancel button click was performed.
After cancellation, authored data and undo state matched the pre-command
snapshot. The saved captures and widget geometry are in the external
[Fillet/Combine audit](../../../Alloy-reference-records/deliverables/rel013-fillet-combine-audit-2026-10-14/audit-01/).

The same fixture was used for Combine with no tool bodies. At 1040×640, the
`tools_required` message was revealed with `QScrollArea::ensureWidgetVisible`
before capture. OK remained disabled and Cancel remained visible. `widgetAt`
hit checks identified the OK and Cancel buttons themselves. The scrolled
target/tool controls were intentionally outside the viewport in this failure
capture, so their hit checks do not demonstrate a control defect. Cancel was
issued through the API, and authored data and undo state again matched the
initial snapshot.

These are bounded empty-selection states: Fillet's no-edge error and Combine's
no-tool error. They do not establish readability for populated or longer
Fillet/Combine states. No product change was indicated by these captures.

## Dimensioned sketch palette

The final presentation-only audit opens
[`dimensioned-rectangle.alloy`](../../examples/dimensioned-rectangle.alloy),
edits its existing sketch, frames it with `sketch.lookAt`, and captures the
palette and canvas at 1360×850 and 1040×640. It then selects one existing line
through `sketch.selectionSet` and captures the selected state. This is API-driven
presentation inspection, not a native pointer-input test. The outcomes and
measured palette/viewport sizes are recorded in the external
[sketch-palette audit](../../../Alloy-reference-records/deliverables/rel013-sketch-palette-audit-2026-10-14/audit-01/).

At 1360×850, the canvas was 792×516 px and the fixed palette was 312 px wide;
its outer scroll viewport measured 286×430 px. At 1040×640, the canvas was
472×307 px and the palette remained 312 px wide; its viewport was 286×221 px.
The content width matched the viewport at both sizes. The sketch's 50 mm and
25 mm dimensions and relation marks are visible in the captures, and the
selected-line capture highlights the selected edge. Finish and Cancel were
visible in both the sketch toolbar and palette footer; `widgetAt` identified
each action itself as the hit owner. At minimum height, palette sections below
the first 221 px are outside the current viewport and require vertical
scrolling; the captures do not show horizontal clipping. After API cancellation,
authored data and undo state matched the pre-edit snapshot.

The minimum selected-state capture also shows always-visible cube accessory
buttons and the UP label overlapping the upper-right sketch and dimension
witness/endpoint area. Visible dimension text does not mean all annotations
are unobscured. NAV-021/#452 owns this observed occlusion; REL-013 remains open.
