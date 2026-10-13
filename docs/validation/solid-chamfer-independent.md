# Equal-distance Chamfer independent review

Reviewed 2026-10-13 on candidate 72f4225 in the equal-chamfer worktree.
Accept bounded SOL-028/#201. The reviewer did not implement the feature.
Broader SOL-005/#50 and the cumulative die tutorial remain separate.

The real application runner `tests/solid_chamfer_api.py` passed preview/cancel,
commit, parameter edit, upstream extrusion height edit, impossible-distance
refusal, undo/redo and exact authored save/reopen. A 20 mm cube chamfered by
1 mm on all twelve edges remains one valid closed retained body, with volume
7885.333333 mm³ and unchanged 20 × 20 × 20 mm bounds. The app exited normally.
This candidate ran successfully with its existing Homebrew Qt 6.11.2 build;
no private-runtime rebuild was necessary for these checks.

The verifier-authored `chamferEdgesDistanceCancelCommit` Qt case passed
(3/3 including setup/cleanup, 1.716 seconds). API creates the stock cube; actual
Chamfer toolbar clicks, two distinct viewport edge clicks, Select all edges,
keyboard distance entry with Enter, and Cancel/OK exercise the real widgets.
Selection reaches twelve edges, preview preserves authored data, Cancel restores
it exactly, and OK creates one Chamfer feature on the retained body. These are
Qt-dispatched events, not physical OS mouse evidence. The API runner separately
covers edit/history/storage rather than repeating those checks in the widget case.

Actual captures at 1180 × 760 show readable edge count, Equal Distance label,
numeric field and both buttons. The solid has visible planar bevels and selected
source edges are blue, including hidden selected edges. The blue source cage
lies outside the modified bevels as expected for source-edge selection. No
matched source-panel image exists, so exact visual parity is unknown.

The public tutorial's drag-window all-edge selection is a recorded mismatch:
this implementation provides individual additive clicks and Select all edges.
That is a human-available adaptation, not an API-only replacement. The die gate
must preserve this qualification; broader selection gestures remain deferred.
The 20 mm cube and 1 mm distance are deterministic fixture choices, not public
source defaults. Two-distance/angle chamfers, tangent-chain variants and generated
bevel-edge naming are outside this accepted child issue.

Evidence is in `build-chamfer/evidence/chamfer-independent/`: `report.json`,
`transcript.json`, `cube-chamfered.alloy`, `chamfer-preview.png`,
`chamfer-committed.png`, `qt-input.txt`, and `qt-preview.png`.
