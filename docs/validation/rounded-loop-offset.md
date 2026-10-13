# Rounded-loop Offset — candidate

SKT-044 / #229, 2026-10-13. Public source ID
`warwick-initials-keyring-20261013`; its creator, URL, retrieval date,
page images and independent contract remain in
`../Alloy-reference-records/knowledge/tutorials/warwick/initials-keyring-independent-contract.md`.
The demonstrated page 3 operation clicks the rounded outline, keeps Chain
Selection checked, enters −1.5 and accepts. Alloy explicitly begins a new
sketch on the actual 2 mm extrusion top face; the public page leaves that
transition implicit. Exact post-fillet driving-size semantics are unknown.

The new generated-only sketch object retains a UUID, the named support
producer/body/face triple, a stable identity derived from the eight authored
outer-boundary source entities, signed millimeter distance and checked Chain.
It stores no copied BRep or once-projected coordinates. Evaluation resolves
the actual named planar face, requires one 4-line/4-arc outer wire with unique
lineage, offsets it with OCCT and creates one exact annular face. Both OCCT
wire orientations are checked so negative always means inward. Zero,
collapsed, multiple-result, nonplanar and changed-lineage cases refuse before
authored commit. The selectable annulus profile ID derives from the retained
offset object's UUID and role, not an edge index.

The shared `sketch.placePoint` path ray-picks an actual top-face boundary edge
and starts a disposable full-loop preview. The palette shows checked Chain
Selection, signed distance, Flip, OK and Cancel; Escape clears pending geometry.
`sketch.loopOffsetPreview`, `sketch.loopOffsetAccept` and
`sketch.loopOffsetUpdate` use that same retained object. Normal sketch
selection and the existing Circle Offset path remain separate. A focused native
test exposed a preview-refresh error: decoration tried to expand the original
face projections against the generated preview's empty projection context.
Decoration now uses the preview draft while pending, and the generated preview
remains onscreen. The palette reports one region and one inner loop.

Focused `rounded_loop_offset_test` passed exact source area
`1769.0973355 mm²`, z=2 cap, inner 57 × 27 mm with four R4.5 arcs, annulus
area `247.4800843 mm²`, opposite signed side, 1.5 mm rim Join volume
`3909.4147975 mm³`, signed-distance and source-radius edits, stable profile
identity, exact undo/reopen and atomic collapsed-offset refusal. The
`rounded_loop_offset_controls_test` passed an actual Qt viewport boundary
click, checked Chain, typed −1.5, preview Escape, re-pick and Return without
test-side focus repair. `tests/rounded_loop_offset_api.py` passed the real
executable's shared command path, preview/Cancel/accept, invalid refusal,
Join, edits, undo and exact saved-model reopen; its private editor exited.

Evidence: `build-offset/evidence/rounded-loop/native/loop-preview.png` and
`loop-accepted.png`; `build-offset/evidence/rounded-loop/api/report.json`,
`loop-preview.png`, `loop-accepted.png`, `joined-rim.png` and
`rounded-loop-rim.alloy`. The inspected native preview shows a continuous
visible annulus with readable Chain and distance controls. The API isometric
capture shows the raised rim and recessed floor. API viewport coordinates are
not physical OS input evidence. Styling has no matched public viewport capture.
The inherited overall 60/30 labels are driven measured references after
filleting; they cannot yet hold authored width through a radius edit (#235).
The complete keyring lesson remains #233.
