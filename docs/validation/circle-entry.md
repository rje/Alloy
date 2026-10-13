# Command-first Center Diameter Circle

SKT-028 #175 is the bounded entry order prescribed by public source `straw-bung`,
University of Warwick WMG Outreach, retrieved 2026-10-12: step1 starts Center
Diameter Circle before choosing XY; steps5 and7 start it before selecting each
top face. Provenance and the step ledger live externally in
`knowledge/tutorials/warwick/`. This changes entry order, not geometric support.

`sketch.circle` and C now start the existing circle tool. Without an active
sketch, the command opens the existing plane/support chooser and exposes
`pendingTool: circle` in the inspectable session. Selecting XY/XZ/YZ or a
supported positive cap begins the disposable sketch with `tool: circle`, clears
the pending tool and focuses the viewport. A preselected supported face or
explicit API `plane`/`support` can enter directly. Within a sketch the same
command activates Circle. The Solid Create menu adds Circle → Center Diameter
Circle; existing Sketch Create Circle uses the same command. Search discovers it.

Origin click, typed diameter and Enter use existing authoritative circle and
constraint authoring. Finish commits the sketch; Cancel discards the chooser or
draft without a feature/history transaction. Escape during support choice
cancels. Escape inside a sketch retains existing tool cancellation behavior;
Cancel Sketch discards the draft. Other shortcut repairs remain outside scope.

Private Qt build passes. The focused native-window Qt widget check
`widget_interactions_test commandFirstCircle` passes global C→chooser, Escape,
menu action→XY, automatic viewport focus, origin click/type26/Enter and draft
Cancel with zero authored features. These are synthesized widget events, not
physical OS input.

`tests/workflows/check_circle_entry.py` passes through the actual app, including
pending choice, chooser/draft cancellation, XY Ø26 circle/3mm extrusion,
base-cap Ø19 circle/8mm Join, Join-cap Ø6.2 circle and exact native save/reopen.
Evidence is `build-circle/evidence/circle-entry.json`, `circle-26.png`,
`circle-19.png`, `circle-6.2.png` and `circle-entry.alloy`. This script uses shared
API pointer/diameter commands. Physical C/menu/top-face entry and independent
review remain pending; no general face support or full tutorial claim follows.

## Native entry checkpoint (2026-10-12)

At candidate `bf48371`, private `build-circle/alloy.app`, macOS26.5.1 on Apple
M4 with private Qt6.11.2, native C opened the pending chooser and Escape canceled
without authored features. C→XY Plane→origin pointer click→type26→Return→Finish
produced the driving Ø26 sketch. After an API3mm extrusion, native C→pointer top
cap→origin→type19→Return→Finish produced the supported Ø19 sketch. After API8mm
Join, native C→pointer new cap→origin→type6.2→Return→Finish produced the third
supported circle. API inspection separately confirms cap frames at z3 and z11,
exact diameters, fixed/coincident origin attachment and fully constrained drafts.
Native sketches remain preserved in `native-circle-entry.alloy`.

Evidence: `build-circle/evidence/native-chooser-cancel.json`,
`native-xy-circle.json/.png`, `native-cap-circle.json/.png`,
`native-join-cap-circle.json/.png`, and `native-finished-circles.json`.
This is native circle entry with API extrusion checkpoints, not an uninterrupted
full native tutorial. No manual canvas focus was required after plane/cap choice.
The inherited Solid Create dropdown did not open under the attempted CUA AX and
coordinate activations; the Circle menu action is verified by synthesized Qt
widget events, while native dropdown invocation remains unknown. Public reference
build/platform and viewport scale are unmatched; broad exact visual parity is
not claimed. Independent review follows using these artifacts.
