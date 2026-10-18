# Sketch entity roles and Find Under-Constrained

SKT-125 / #550, accepted 2026-10-18 after independent Sol review. Public inputs and verification contract:
external `knowledge/specs/skt-023-sketch-freedom-presentation-split-2026-10-18.md`,
P1/P2 (rechecked 2026-10-18). The external selection-status knowledge record now
records that newer direct documentation establishes the five semantic categories.
Exact RGB values, matched screenshot parity and reference precedence remain unknown.

## Workflow contract

The shared solved sketch diagnostics expose `entityRoles` and
`underconstrainedEntities`, with stable `entityId`, geometry `type`, semantic
`role`, `fullyConstrained`, and `analysisKnown`. Aggregate sketch `dof` remains a
separate solver result. No numeric per-entity DOF is introduced or saved.

Unconstrained ordinary curves use blue; unconstrained construction and centerline
use orange with their existing dash patterns; linked projections remain purple;
immovable curves whose owned points all carry explicit Fix relations use green;
other fully constrained geometry uses black. Fix precedence over fully constrained
is Alloy policy. This extends existing point-only Fix; it does not add a new
whole-curve Fix command. A partially fixed curve remains under-constrained.
Selected/hover presentation remains separate from its semantic role.

Find Under-Constrained opens a session-only live palette list (initially closed).
Clicking a row or moving keyboard selection selects and highlights the same
stable authored entity and fits its displayed BRep bounds. Accepted relation or
dimension edits, Undo/Redo, and source re-evaluation refresh diagnostics/list.
The native list and API use `sketch.underconstrainedSelect`. Framing reuses cached
displayed entity handles; it does not re-evaluate the sketch or document.
`sketch.findUnderconstrained` opens/closes the list and `sketch.compute` refreshes
analysis without an authored edit. Inspection is immediate, has no preview or
commit stage, and adds no undo entry. Finish/Cancel retains existing sketch rules;
list visibility is not authored or persisted. Invalid/stale row IDs are refused.

`analysisKnown=false` preserves existing conservative freedom limits for derived
geometry. Such rows stay in the list with “analysis limited”, rather than claiming
that unsupported analysis proves immobility. Spline handle and conic parameter
limits remain those of the existing solver. Text/generated-only sketches retain
their separate evaluator diagnostics. Source edits follow the existing historical
edit lifecycle; an externally changed active session is refused as stale.

## Retained verification

Implementer checks pass in `build-preview-release` (Release, patched Qt 6.11.2,
OCCT 7.9.3, arm64 macOS 26.5.1). The focused core check passes, the actual-app
API workflow passes, and native Qt reports 3 passed / 0 failed / 0 skipped.
GPU evidence reports Apple M4 / Apple / OpenGL 4.1 Metal - 90.5, display scale 2,
1744 × 1132 native viewport framebuffer in the 1440 × 900 API window.
The API full-view, selected/zoomed and native-list captures were inspected:
blue free line, dashed orange construction, green fixed line, black dimensioned
circle, purple linked edges, readable list and selected row. `summary.json`
records the executable hash and detailed evidence links. The private editors
were closed after validation.


External evidence root:
`../Alloy-reference-records/validation/sketch-entity-roles-2026-10-18/`.

- Focused `entity_solved_colors_test` checks independent solved/free construction
  axes in a mixed ellipse/conic sketch and refresh after dimension removal.
- `tests/sketch_entity_roles_api.py` creates the mixed fixture through the real
  executable/local control server: free ordinary line, free construction line,
  a line fixed at both endpoints, a dimensioned circle, and linked support edges.
  It checks the semantic categories independently of total DOF, list selection,
  inspection without authored/history changes, relation removal and explicit
  reapplication, draft and committed history, actual projection source edits,
  authored save/reopen, hardware diagnostics, and GPU captures. The supported-face
  sketch retains its four automatic boundary projections.
- `tests/sketch_entity_roles_controls.cpp` reuses that saved model. Set
  `ALLOY_ENTITY_ROLES_FIXTURE` to `api/mixed-roles.alloy` and
  `ALLOY_ENTITY_ROLES_EVIDENCE` to the native evidence directory. It waits for
  native Open completion, performs initial activation once, then clicks Find,
  clicks the actual list viewport row, presses Down, clicks Compute and closes
  the list without focus/selection repair. Qt event synthesis is distinct from
  physical OS input. No proprietary application is operated.

Independent review accepted the bounded API, native, source/history and actual
GPU evidence without a material blocker. These checks do not establish
Windows/Linux runtime or exact visual parity. Additional visibility controls and broader relation glyph/edit behavior
remain the separate SKT-023 children.
