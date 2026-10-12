# Sketch controls compatibility record

Reference review: 2026-10-11. Scope: origin-plane entry, line/rectangle drawing,
Sketch Palette, constraints/dimensions, Finish/Cancel and re-entry. This is a
bounded evolving workflow record, not full sketch parity. Follow the
[milestone review process](../../compatibility.md).

## Permitted reference baseline

Existing source IDs are in [sources.json](../../backlog/sources.json); original
public URLs and images stay outside the repository in
`../Alloy-reference-records/`, relative to the repository root. The focused
[reference evidence record](sketch-reference-evidence.json) adds current public
documentation/community/tutorial observations and timestamps. Source findings
remain distinct from Alloy's implementation policies.

| Source IDs | Documented or observed behavior | Still unknown in this baseline |
| --- | --- | --- |
| `sketchmode` | Create Sketch chooses an origin/work plane or planar face; existing sketches re-enter through Browser/Timeline. Sketch context and Finish Sketch have blue emphasis. | Exact plane-picker layout, selection precedence and platform-specific focus behavior |
| `palette` | Options depend on the active tool/selected object; Look At views normal to the plane. Grid, Snap and geometry/dimension/constraint visibility controls exist. | Initial checkbox values in each context and exact full-palette spacing/styles |
| `keys` | L Line, R Two-Point Rectangle, D Sketch Dimension, S toolbox; Mac undo/redo use Cmd+Z/Cmd+Shift+Z. | Sketch Escape/cancel transaction semantics and every text-entry focus transition |
| `dimensions` | Curves are selected and dimension placement is chosen in the canvas. Driving dimensions control geometry; driven measurements update from geometry and use parentheses. | Exact overconstraint prompt/conversion UX; public wording describes automatic/optional driven behavior differently |
| `official-sketch-rectangle`, `community-sketch-001`/`002`/`003` | Public original articles/transcripts describe automatic H/V constraints on two-point rectangles, D → select geometry → choose placement → enter value → Enter, persistent tools stopped with Escape and double-click dimension editing. | Videos were not played or timestamped in this pass; article/transcript instructions are documented evidence, not visually observed runtime states. |

Original public community articles and video tutorials may fill these gaps.
Record source/creator, retrieval date and timestamps for the interaction actually
shown; do not infer keyboard presses or hidden defaults from silent footage.

## Alloy evidence and current differences

The last accepted [origin-plane sketch slice](../../validation/sketches.md)
provides real plane transforms, connected lines/four-edge rectangles, shared
Finish/Cancel authority, undo and native save/reopen. Its fixtures and separate
native checks are the reusable starting evidence. The integrated constraint and
dimension subset has its own [candidate-specific receipt](../../validation/sketch-constraints.md).
Automatic H/V, supported relations, driving/driven values, failed-draft preservation
and storage/history passed bounded app verification. The corrected binary also
passed the focused case and native selection/dimension-entry checks. That output
evidence does not establish canvas placement or prompt parity.

| Concern | Review status and follow-up |
| --- | --- |
| Origin plane, retained entity IDs and one-entry Finish/Cancel | Alloy outputs verified for the declared subset. Face support stays SKT-013; exact reference cancellation semantics remain unknown. |
| Start X/Y and End X/Y palette fields | Verified Alloy coordinate helpers. **Mismatch** with the documented inline rectangle width/height and Tab workflow; exact current focus/layout details remain unknown. [SKT-005 #32](https://github.com/rje/Alloy/issues/32) owns the inline entry correction. |
| L/R/D and pending drawing Escape | Alloy controls verified in the declared native slices. Shortcuts agree with the public table, and stopping a drawing tool with Escape agrees with community instructions. D currently uses palette numeric entry; complete Escape/cancel transaction parity remains unknown. |
| Constraint/dimension selection, placement and editing | Point/line picking, numeric palette entry, driving edits and driven readouts are implemented. **Mismatch** remains for D → canvas placement → value/Enter, double-click editing and the driven-conversion prompt. [SKT-005 #32](https://github.com/rje/Alloy/issues/32) owns these bounded corrections; functional equations do not establish interaction parity. |
| Automatic rectangle H/V | **Matched** documented behavior for new rectangles: four H/V constraints are authored together. Document/API checks verify those records and the remaining DOF after anchoring. Existing saved rectangles are preserved as authored; the constraints receipt identifies the tested candidates. |
| Palette layout, semantic colors and full edit workspace | Orange selection, green fixed points and whole-sketch black/blue status exist. Per-entity freedom badges/colors and visibility remain [SKT-008 #35](https://github.com/rje/Alloy/issues/35). Exact layout/style is **unknown** without a matched public state; a toolbar crop cannot certify it. |

Keep concrete fix links in this table as differences become evidenced; SKT-003/004
own their command behavior, and FND-001 owns the dated reference baseline. A known
helper gap does not block functional modeling progress or reduce the eventual
compatibility target.

## Next bounded review fixture

Review one rectangle → constraints/dimensions → Finish → edit → undo/reopen
candidate. Inspect stable point/entity/constraint/dimension IDs, solved 40 × 25 mm
geometry and the width edit to 50, with analytic output checks separate from
visuals. Use the API fixture for deterministic state and a short native pass for
selection, keyboard/focus, placement and acceptance. Compare screenshots only
where reference state, scale and viewport can be matched; record remaining
differences and unknowns with specific evidence and bounded fix issues. Do not
repeat the complete unchanged app workflow just to produce another capture.
