# Command-first Center Diameter Circle

SKT-028 is the bounded entry order prescribed by public source `straw-bung`,
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
