# Primitive editing and navigation references

Retrieved 2026-10-11 from permitted public documentation and screenshots. URLs
and the public Box image are retained separately in
`../Alloy-reference-records/primitive-editing.json`; stable reference IDs below
are in docs/backlog/sources.json. Reference images are never application assets.

| Evidence | Finding | Limit |
| --- | --- | --- |
| `ui-command-box` public screenshot | Compact Box command inputs sit beside the canvas on its right; height is also shown on canvas | Different viewport size/theme/state; no exact visual parity claim |
| `ui-solid-box` documented instructions | Choose a plane and two corners, then set height using a manipulator or dialog; operation choices are documented | Alloy currently uses world-XY numeric placement and New Body; those interactions remain future work |
| `ui-command-docking` public instructions | Command dialogs can be docked or undocked | Alloy implements independent Qt docking; dimensions/spacing are still an approximation |
| `ui-trackpad-navigation` public guide | Trackpad gestures include two-finger pan, shifted orbit and pinch zoom | Device/preset differences and a contradictory button illustration prevent declaring exact gesture compatibility |
| `qt-docking` Qt documentation | QDockWidget supplies docking, floating, movable panels and a close action | Native behavior still requires actual Alloy verification |

The centered modal primitive editor was a temporary Alloy implementation, not a
researched compatibility result. The new editor uses a QDockWidget scoped to the
canvas: right-docked initially, floatable/redockable and modeless. Committing a
field updates the disposable preview through shared commands. Editing previews
hide the original presentation until cancel/failure/stale-session restoration,
so reductions in size are visible. Selection uses a distinct blue shade.

Option/Alt plus left-drag is an Alloy usability binding for users without a middle
mouse button. Middle drag and wheel zoom remain supported. This addition is not
an exact reference-preset claim. Face/edge selection, arbitrary orbit, native
trackpad navigation, placement picking and manipulators remain tracked work.
