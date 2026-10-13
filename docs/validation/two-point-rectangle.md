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
size parsing refuses Accept and Cancel/tool change clears the typed input;
these two refusal branches were not separately exercised in the runtime cases.
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
The inspected `rectangle-preview.png` visibly contains the full right/down
outline and persistent Width 60 / Height 30 labels; `rectangle-committed.png`,
saved model and `state.json` are adjacent. Pointer movement is an injected Qt
event on Alloy's real view, not a physical OS mouse receipt.

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
