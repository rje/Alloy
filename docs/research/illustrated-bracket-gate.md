# Exact illustrated bracket gate

[TUT-003 #155](https://github.com/rje/Alloy/issues/155) is blocked. The 2026-10-12 audit against source commit `1c5749e`
reviewed all three public activities and their images. It did not execute the
complete tutorial in Alloy. Existing geometry primitives are not step passes.

The external reference ID is `tutorial-illustrated-bracket`. Detailed action
locators, current capability classifications, verification instructions and source
URLs are in `../Alloy-reference-records/knowledge/tutorials/illustrated-bracket/audit.json`.
`visual-sources.json` maps 25 scene-image occurrences to 22 unique screenshots.
It also records inline command icons, including unavailable source URLs. Toolbar
icons are not evidence for in-canvas badges; further glyph research remains required.
Research assets remain outside the repository and are not application assets.

## Exact workflow contract

- Activity 1: new document via + tab; XY sketch; origin-based connected outline;
  constraint-first repeated pair selection; line Equal; placed lengths 500 and
  100 mm plus a genuine 45-degree two-line angle.
- Activity 2: context-menu construction conversion; midpoint/inferred circles;
  two-corner sketch fillet of radius 100 mm; three-region Press Pull at 50 mm;
  face-supported sketch and construction lines constrained to model edges.
- Activity 3: five reference-snapped circles; inline initial diameter 50 mm;
  four successive Equal pairs; five-profile Press Pull Cut using its arrow;
  platform-specific profile selection toggles.

Preserve the documented tools, order, values, selections and edits. Activity 1
explicitly permits different initial automatically inferred constraints before
the manual constraint steps; that permitted variation is not a gate failure. No separately
extruded cutter bodies, Combine substitutions, fixed copied coordinates or
prebuilt geometry count as tutorial completion. The source leaves cut distance
unspecified: verify a complete through-cut via the demonstrated drag; do not
invent a required Through All option. Cloud-dependent features may be deferred
under the product contract; none of these modeling steps needs that exception.

## Visual and interaction acceptance

Geometry-only success cannot close this gate. Capture each completed constraint
stage and all three activity endpoints, plus the active fillet and extrusion
previews. Compare with the corresponding external reference images.

- Constraint badges must represent real retained relations on the intended
  geometry. Check perpendicular, horizontal/vertical, equal, midpoint,
  tangency, radius and parallel indications as applicable. Distinguish transient
  inference cues from retained badges; a generic decorative marker does not pass.
- Check glyph identity, readable contrast/scale, anchor placement, collisions,
  occlusion and selection/hover feedback. Removing a relation must remove its
  marker; undo and sketch re-entry must restore the correct presentation.
- Check constraint/dimension visibility controls as a supplementary visual check.
  Their exact behavior needs additional public evidence because these tutorial
  steps do not demonstrate show/hide toggles. Restore the tutorial-visible state.
- Check origin attachment, constrained/underconstrained colors, dashed construction
  lines, profile fill, selected-profile colors, annotation text/units, extension
  lines/arrows, aligned dimensions and angular sector placement.
- Check command prompts, active-tool state, preview highlights, extrusion arrow,
  numeric entry and resulting solid silhouette/five openings.

Use comparable camera orientation, crop and model scale. Source build, display
scale and some transient states are unknown; record those limits instead of
claiming pixel equality. Resolve the source's ambiguous square/tangency indicator
wording with additional permitted evidence before implementing its icon behavior.
Alloy branding and independently authored assets remain required.

## Verification and ownership

Use native Alloy input for command-first selection, inferred snaps, inline typing,
right-click menus, arrow dragging and platform modifiers. Use its local control API
for shared state, authored relations, geometry and save/reopen evidence. An API
shortcut cannot prove the corresponding mouse/keyboard step. Each numbered source
action receives passed, mismatch, blocked or unknown status and an evidence link.
Astra independently reviews the actual captures and result. Add one upstream edit,
undo/redo and reopen check as explicitly Alloy-added persistence checks.

Existing owners: SKT-005/006/007/008/009/010/013/016, DOC-006 and SOL-015.
New bounded owners: DOC-015 (document tabs), SKT-018 (constraint-first sessions),
SKT-019 (sketch fillets), SOL-016 (profile Press Pull/multi-selection/drag), and
TUT-003 (this cumulative gate). TUT-003 is a prerequisite of GATE-020.
The workflow runner must reject this unimplemented case until an executable
recipe exists. No test or UI-fidelity pass is claimed by this planning audit.
