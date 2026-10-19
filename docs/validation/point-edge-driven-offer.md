# Point-edge redundant Driven offer — SKT-167 / #613

2026-10-19 candidate. The existing retained point-to-edge implementation and
independently accepted two-offset workflow remain documented in
[the original receipt](point-edge-dimensions.md) and
[its review](point-edge-dimensions-independent.md). This increment verifies the
point-edge-specific redundant Driven offer and prepares the stricter native
input gate. Native acceptance remains pending; #613 is not complete.

No production changes were needed. UI and API already route through
`publishNewDimension`, the solver-backed `drivenAlternative`, and the shared
`sketch.dimensionOfferAccept` / `sketch.dimensionOfferCancel` commands. A matching
numeric redundant driver offers Add Driven; a conflicting value refuses. The
accepted measurement retains the circle-center and projected straight-edge IDs,
contains `driving:false` and no authored value, and adds no constraint equation.
Its measured value follows the driving source dimension. Editing a measurement
refuses; its inline edit session reports read-only intent.

The external clean-room contract is
`../Alloy-reference-records/knowledge/specs/skt-016-point-edge-distance-2026-10-19.md`.
Alloy measures positive perpendicular distance to the infinite supporting line
and retains the initial oriented side for drivers. Finite-segment, endpoint,
negative/zero and source-side parity remain unknown/outside this increment.
The observed tutorial places then types; current general public help describes
typing before placement. Alloy's verified route here remains place then type;
no universal input-order parity is claimed.

## Evidence

The accepted `build-preview-release` cache identifies Release and this source
root. Local application, focused core and native-controls targets built with
`-j4`; no CI ran. `point_edge_dimensions_test` passed its existing retained-edge,
Cut, upstream rebuild, history and storage checks plus new point-edge duplicate
redundancy, unchanged geometry, exact measured-reference retention, source driver
5→6 measurement update and conflicting-value no-offer checks. An existing test
lookup retained a dangling `QJsonValueRef` from a temporary JSON object; changing
that test-only lookup to `.value()` removed its validation crash.

`tests/point_edge_driven_offer_api.py` passed 58 calls through the actual app.
On the 31.8 × 15.8 × 9.6 stock face, a Ø5 circle receives two retained 3.9 mm
point-edge drivers. A duplicate matching top-edge driver offers a measurement;
Cancel preserves exact draft/history, conflicting 4.1 refuses without an offer,
and Add Driven adds exactly one history step without changing geometry or
projections. Undo/Redo restore exact drafts. The read-only measurement rejects
numeric edits and follows its original driving dimension 3.9→4.9, moving the
center to (3.9,10.9). Its authored identity/references remain unchanged and exact
save/reopen preserves both authored state and the 4.9 mm measured result.
The owned application exited normally.

Inspected actual framebuffer capture `api/point-edge-driven-4.9.png` shows the
readable driving 4.9 and 3.9 labels and separate blue `(4.9)` measurement with
witnesses on the shaded stock. The app reports Apple M4, Apple vendor, OpenGL
4.1 Metal - 90.5 and display ratio 2. The capture is isometric after Home;
matched public viewport/scale and exact visual parity are not established.

Evidence is external under
`../Alloy-reference-records/deliverables/skt167-point-edge-driven-2026-10-19/`:
`core/test.log`, `api/report.json`, `api/transcript.json`, `api/state.json`,
`api/point-edge-driven.alloy`, and the framebuffer capture.

## Pending native gate

`point_edge_controls_test` is hardened to require initial active-window/focus
readiness, actual-hit viewport widgets for D's subsequent center/edge/placement
and label-drag clicks, the native focus produced for inline value input, editor
closure and naturally returned viewport focus. It asserts production selected
the entire inline value instead of repairing text selection. Escape requires an
observed placing state, clears pending input and preserves the draft. Its former
focus repair before Escape is removed. It compiles, but was deliberately **not run** because the
macOS console is locked. This is prepared verification, not accepted native
input evidence. Run the focused case once the console preflight succeeds;
independent review remains required for this increment. No broader tutorial
replay, physical OS input, release/platform or parent-family acceptance is claimed.
