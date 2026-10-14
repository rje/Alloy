# Linked local occurrence subtree checkpoint

ASM-017/#366 is independently accepted at `de30222` (UI source `70cfd2e`).
This is an unjointed component checkpoint,
not acceptance of copied assembly motion.

`component.copyOccurrence` takes a source `occurrenceId` and absolute parent-local
`placement` (`axis`, `angleDeg`, `translationMm`). Translation follows rotation:
the local transform is T*R; ancestry composes parent*child. The result returns
`rootOccurrenceId` and an old-to-new `occurrenceMap`. New occurrences share source
component definitions, authored features and source body/edge identifiers.
Copied descendants retain explicit parent-occurrence identities.

`component.inspect.localInstances` provides occurrence-qualified paths, composed
world poses and placed body bounds. Source edits affect every instance. Body and
edge selection/projection require an occurrence ID when a source body is shared.
The native Copy Component dialog exposes placement; its default translation is
zero. The piston gate explicitly enters -180 mm X.

## Evidence

- Authority source `d31a018`, integrated as `d0657f5`, passed independent Astra
  review. Focused core evidence checks shared definitions, actual placed BRep
  bounds, a source-height edit reaching both copies, history/reopen and
  noncommuting transform composition. Cyclic parenting refuses atomically.
- Native test passes 3/3: Browser Copy, typed -180 mm, Cancel/OK, original and
  copied Browser rows, exact Qt viewport body clicks, Move and copied-face Sketch
  refusal. Initial harness activation and direct document setup are separate from
  these Qt inputs. This is not physical OS input parity. The GPU capture shows
  both pistons and the copied Outer body highlighted blue.
- Independent review found and verified fixes for resolved Move defaults,
  selected-face Sketch creation and active Split/Pattern picks losing occurrence
  identity. Split/Pattern guards are source-reviewed only. The final removal of
  copied-row checkboxes is source/build verified, not separately visually tested.
- `tests/linked_subtree_api.py` is the real-app workflow gate for shared source,
  placement, selection immutability, edit/history/reopen and explicit export
  refusal. API-02 passes 31 calls including three intended refusals. API-01 stopped
  because the test omitted required export refinement arguments; corrected inputs
  exercise the intended copied-design refusal. Final reopened geometry was
  visually inspected; Browser copy rows are below the visible scroll region.
- Integrated main application builds and focused linked-subtree core passes 1/1
  in 0.85 seconds. No broader compatibility claim is implied.

External evidence: `deliverables/linked-subtree-2026-10-14/`.
Native evidence: `deliverables/piston-linked-copy-2026-10-14/native-08/`.
The external gallery links the editable model and captures at `#piston-linked-copy`.

Joint-containing copy remains unsupported until ASM-016/#365 remaps the internal
joint and verifies the second piston loop. Occurrence-aware STEP/STL is tracked
in ASM-018/#368; current copied-design export must explicitly refuse rather than
silently omit copies or their placement. These follow-ups remain dependencies of
the complete piston tutorial gate.
