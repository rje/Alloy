# Native Selection Priority candidate

DOC-025 adds a Select > Selection Priority submenu for Body, Face and Edge.
Exactly one mode is active; choosing it again returns to Auto. The same
`selection.priority` command drives the menu and local API. Ordinary canvas
`selection.pick` uses the existing body, retained planar-face or current
solid-edge pick authority for the active mode; a miss does not fall back to
another kind. A selected edge retains its body/edge identity in `app.state`
and uses the existing blue solid-edge highlight. Active modeling commands keep
their explicit picker role while the ordinary priority remains stored.

Auto at startup and window-local, non-authored persistence across Cancel and
subsequent commands are Alloy choices. The public source documents menu
paths, exclusive modes and active-mode toggle, but does not establish its
startup priority or save/reopen behavior. The research and tutorial step are
recorded externally under
`../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-13/selection-priority-contract.md`.
Component priority, general selection filters, multi-selection and boundary
selection remain outside this child.

The app and focused native target build pass. Qt-delivered native menu clicks
activated Edge Priority; two actual front-view Box edges were clicked in the
canvas and returned retained edge IDs rather than faces. The selected bottom
edge has a readable blue GPU cue and the toolbar displays `SELECT · EDGE`.
The same native sequence selected Face Priority, clicked a supported face,
and toggled that mode back to Auto. The final inspected capture is external
at `../Alloy-reference-records/deliverables/selection-priority-2026-10-13/native-04/native-edge-priority.png`.

The 33-call real-app API ledger at
`../Alloy-reference-records/deliverables/selection-priority-2026-10-13/api-03/`
checks Edge/Face/Body dispatch, stable selection state, atomic edge misses,
the explicit Fillet edge picker while priority remains Edge, Cancel retaining
that mode, toggle back to Auto, and unchanged authored Box data. Native and
API runs used one private editor at a time and exited cleanly. The menu
gesture was Qt-delivered; no OS-level pointer claim is made.

A focused follow-up real-app ledger at
`../Alloy-reference-records/deliverables/selection-priority-2026-10-13/api-transitions-01/`
checks that a selected ordinary edge is replaced by construction-plane,
sketch and profile selections, and does not leak through a document-tab switch.
Those transitions clear the old edge cue; the authored geometry remains under
the same document authority. This follow-up passed and its editor exited.
