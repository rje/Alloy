# Typed chained Line length lock candidate

SKT-040 / #222, candidate 2026-10-13. Public source ID
`sweep-paper-clip-warwick-2017-20261013` was independently reviewed on
2026-10-13; source URLs, public page images and exact source ledger remain in
`../Alloy-reference-records/knowledge/tutorials/next-sweep-slice/paper-clip/`.
The documented interaction is L, click origin, type a length, Tab lock, point
and click to continue seven H/V segments, then Escape. The source does not
establish the underlying retained constraint types or a fully constrained
status. Alloy deliberately retains an H/V relation and a driving H/V dimension
per placed segment. The value lock, pointer direction and uncommitted start are
session-only; they disappear on cancel/tool exit and never enter saved JSON.

The bounded implementation shares `sketch.lineLengthPreview`,
`sketch.lineLengthLock`, `sketch.pointerMove` and `sketch.placePoint` between
the native editor and local control API. An invalid or out-of-range value
refuses Tab lock and click without changing the draft. A locked value previews
the dominant horizontal or vertical pointer direction; a small viewport cue
shows the length, units and H/V. Placement atomically authors a line, relation
and driving dimension, and keeps its solved end as the next start. Escape ends
the chain. Changing tools or canceling clears the pending lock. Lines, their
dimension values and their exact endpoints survive sketch history and reopen.

`tests/typed_line_controls.cpp` delivered actual Qt widget key and pointer
events on the real OCCT view. It passed 1/1 in 7.68 s at 1400×900 logical
window size and 2× device scale, including L, origin click, invalid zero,
typed values, Tab, moving previews, click continuation, seven exact endpoints,
Escape, draft undo/redo, and save/reopen. It also checked the existing typed
Circle Return path and fillet tool entry after the chain. Captures
`build/evidence/typed-line/locked-1.png` and `locked-5.png` show the first and
fifth native previews; the latter visibly includes the 44 mm · V cue and
existing 40/14/60/11 dimensions. The final native state reports Apple M4,
Apple OpenGL 4.1 Metal, hardware acceleration, 1664×1142 framebuffer and 4×
MSAA. This is injected Qt input, not a physical keyboard/mouse receipt.

`tests/typed_line_api.py` passed against the `alloy` executable with 72 shared
control calls. It uses projected pointer positions to choose direction, then
places through the same command; it does not seed each line with direct
coordinates. The exact source endpoints and 40/14/60/11/44/8/24 mm values
match the external contract. Authored JSON has seven lines, seven driving
dimensions and fifteen relations (seven H/V, seven adjoining coincidences,
one fixed origin); invalid zero leaves the draft unchanged. Draft undo/redo and
exact save/reopen passed. Evidence is local under
`build/evidence/typed-line-api/result.json` and its control transcript.

Compatibility status: **matched** for the bounded documented Line key/value/
Tab/direction/click/Escape sequence and source geometry under injected Qt
events; **unknown** for physical OS input and exact reference styling/constraint
storage. The current Circle editor accepts a typed diameter with Return, but a
separate Qt probe of source Circle Tab→Enter moved focus away from the editor
before Enter (`QApplication::focusWidget()` was null after Tab). The bounded
follow-up is SKT-041/#226 before full tutorial acceptance. Paired shared-line fillets, native Sweep and the complete object
remain separately tracked. Broader rectangle/inference work stays in #162.
