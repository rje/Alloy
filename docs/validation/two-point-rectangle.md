# Typed two-point Rectangle candidate

SKT-042 / #227, 2026-10-13. The permitted six-page lesson is indexed externally
as `warwick-initials-keyring-20261013`, reviewed and retrieved 2026-10-13.
Source URL, page images and detailed uncertainties remain in
`../Alloy-reference-records/knowledge/tutorials/warwick/initials-keyring-independent-contract.md`.
The documented actions are command-first 2-Point Rectangle, XY/Top, origin,
60 × 30 in two live boxes with Tab, and Enter twice. The source image shows a
right/down rectangle. Input-box order and the internal meaning of the two Enter
presses are unknown. Alloy labels Width and Height separately; the first Enter
places the rectangle, and the second exits its tool. Pointer motion after the
origin selects the quadrant, rather than assuming the screenshot's direction.

`sketch.twoPointRectangle` uses the existing sketch plane chooser and Rectangle
tool. `sketch.twoPointRectanglePreview` keeps the typed dimensions and selected
quadrant only in the active session; the GPU cursor shows the resulting bounds.
Accept uses the existing four-line rectangle authority and publishes both
driving dimensions with the corner-origin coincidence in one solved draft edit.
No pending value enters saved data. Source inspection confirms invalid positive
size parsing refuses Accept and Cancel/tool change clears the typed input.
The direct coordinate `sketch.rectangle`, Center Rectangle, Circle and Line
remain available through their existing commands and editors.

`tests/two_point_rectangle_controls.cpp` passed the focused Qt sequence
`sourceRectangle` in 3028 ms (3 stages passed, 0 failed). It activates the
initial test window, selects the command before XY, clicks the origin, delivers
a Qt mouse-move event into the right/down quadrant, types 60, Tab, 30, Enter,
and Enter without repairing focus between those actions. The test asserts
viewport-to-Width-to-Height-to-viewport focus transitions, disposable preview,
four edges, driving 60/30 dimensions, draft undo/redo and exact save/reopen.
After the source sequence, a bounded shared-editor check opens Circle and Line
input and confirms their value field is editable with the Rectangle labels
hidden. `build/evidence/two-point-rectangle/native.log` is the persisted log.
The inspected `rectangle-preview.png` visibly contains the right/down outline
and persistent Width 60 / Height 30 labels. The editor covers part of the
lower-right outline in this 1400 × 900 capture; the authored bounds and closed
profile are verified from the model, not from an unobscured preview. The
`rectangle-committed.png`, saved model and `state.json` are adjacent. Pointer
movement is an injected Qt event on Alloy's real view, not a physical OS mouse
receipt.

Independent review found the Sketch Palette summary compressed to 15 logical
pixels even though its current text needed 60; the profile status likewise
needed 30. The dynamic labels now update their minimum heights from their text
size hints. The focused `invalidCancelAndPalette` Qt case passed in 1556 ms
(3 stages, 0 failures): typed 0 × 30 followed by Return leaves the draft and
history unchanged, keyboard correction to 60 × 30 gives a preview, and Escape
clears the pending input, labels and GPU cursor without authoring. A subsequent
Circle input is editable. The test changes focus only through normal Tab,
Shift-Tab and tool actions after initial harness activation. In
`build/evidence/two-point-rectangle/invalid-cancel.log`, the live summary and
profile labels each have their required height (60 and 30 logical pixels),
with no overlap. The inspected `rectangle-cancel-preview.png` shows readable
palette summary/profile and normal spacing before selection guidance. Its
lower-right outline is likewise behind the inline editor. The original
`rectangle-preview.png` is retained as pre-correction evidence of clipping.

`tests/two_point_rectangle_api.py` passed against the Alloy executable with 24
shared command calls. It uses projected origin and pointer positions, previews
without changing the draft, accepts, switches to Select, undoes/redoes the
single edit, then saves and reopens. The saved XY sketch has bounds
x=0..60 mm, y=−30..0 mm, four lines, driving horizontal60/vertical30
dimensions, an origin fixed relation and first-corner coincidence. Evidence is
`build/evidence/two-point-rectangle-api/result.json` and its saved document.
The API fixture establishes command/storage behavior, not the native key path.

Compatibility status: **matched** for the bounded demonstrated command order,
dimension input, right/down shape and two Enter completion under injected Qt;
**unknown** for physical OS input, exact reference styling and the reference's
retained constraint semantics. Angled modes and broad inference remain with
the parent sketch issue; the full rounded keyring lesson is separate.

## Independent acceptance

Accepted `89c5d58` with corrective candidate `ffefc014` on 2026-10-13.
Independent review covered the shared authoring/inline-input implementation,
original native sequence and API evidence, saved authored rectangle, and the
corrective source, persisted 1556 ms native log and actual corrected capture.
No additional editor or unchanged full replay was needed.

The corrective test checks natural focus at each key transition and sends keys
to the focused widget; it does not insert a focus repair between the demonstrated
steps. Invalid Return preserves draft/history, ordinary keyboard correction
previews valid dimensions, Escape disposes the inputs and cursor, and the next
Circle editor remains editable. Initial harness activation and API fixture/tool
setup are explicit, distinct from these tested input transitions.

The earlier palette clipping is resolved in the reviewed capture: summary and
profile text are readable with appropriate spacing. Width/Height labels and
values are legible. The inline editor still overlaps part of the lower-right
preview edge as disclosed above; the actual authored bounds, closed profile and
retained dimensions supply the geometry evidence. This is bounded Qt-dispatched
input and real shared-API/storage acceptance, not physical OS input or exact
reference focus/styling parity. Full lesson and rounded-outline behavior remain
separate gates.
