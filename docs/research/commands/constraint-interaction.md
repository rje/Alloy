# Constraint interaction slice

Functional slice verified, 2026-10-12; owners SKT-008 and SKT-018.
See [verification and remaining limits](../../validation/constraint-interaction.md). Source evidence
is external `knowledge/specs/constraint-badge-visibility-delete-undo.json`.
Public instructions establish Show Constraints/Show Dimensions controls and
constraint deletion by hover/Delete or glyph selection/Delete/context menu.
Exact target highlighting and active-editor display persistence remain uncertain.

## Bounded contract

- Existing authored constraint IDs drive glyph selection and deletion. Selecting
  a glyph is not a separate geometry model. Keep dimension label dragging and
  double-click editing intact; active drawing/constraint tools still pick their
  intended geometry without decorative glyphs intercepting those picks.
- Display options belong to the active sketch session, default visible, and make
  no authored CAD edit or undo entry. This is an explicit initial Alloy policy,
  not a claim about reference save/reopen persistence. Hiding a class removes its
  presentations and hit targets, and clears that class's selection/hover so Delete
  cannot unexpectedly act on an invisible item.
- Selected and hovered constraint IDs are inspectable. Target highlighting is an
  Alloy usability choice pending more precise public visual evidence.
- Delete removes selected authored relations, or the hovered constraint when no
  relation is selected. Existing single dimension selection may use the same
  bounded deletion path. Text editors retain their normal Delete behavior.
- One deletion is one accepted draft edit: validate all IDs, solve the candidate,
  publish atomically, and use the established draft undo/redo stack. A rejected
  solve or stale ID preserves the previous draft. Removing a driving equation
  releases only the DOFs left free by the other retained relations.
- No generic geometry deletion, solver rewrite, complete constraint-family
  expansion or per-entity freedom classification is included in this slice.

## Shared interface and verification

Implemented commands: `sketch.displayOptions`, `sketch.constraintSelect`,
`sketch.constraintHover`, `sketch.constraintPick`, `sketch.deleteSelection`.
State exposes `activeSession.displayOptions`, selected constraint IDs and hovered
constraint ID. Final schemas are discoverable from the running command registry.
Native input and API call those same handlers; the renderer supplies actual
viewport-local glyph bounds, not a duplicate hit-test model.

Sol core owns session/commands/palette and focused deletion/history checks;
Sol UI owns rendering/hit-testing/native glyph interaction. Root integrates and
runs a short real-app native workflow; Astra verifies source and actual captures.
Check a visible relation deletion, observed solver change and undo restoration,
independent visibility controls, hidden targets, and unaffected dimension editing.
Reuse accepted bracket geometry evidence rather than repeating the full tutorial
for unchanged modeling behavior.

## Remaining glyph evidence

The external evidence now distinguishes point-to-point Coincident (documented
without a persistent icon) from point-to-curve Coincident (documented icon).
The origin-specific case remains uncertain. Fixed geometry locking and green
feedback are documented, but its exact persistent glyph styling is unknown.
Missing glyphs must therefore be assessed per relation/context, rather than
assuming every coincident point requires a permanent icon. This slice does not
implement a contextual relation-reveal interface or expand Fixed targeting.
