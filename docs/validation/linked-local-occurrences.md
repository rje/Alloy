# Linked local occurrence subtree checkpoint

ASM-017/#366 is in progress. Authority is independently accepted; native/UI and
actual-app evidence are pending. This is an unjointed component checkpoint,
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

## Evidence and remaining acceptance

- Authority source `d31a018`, integrated as `d0657f5`, passed independent Astra
  review. Focused core evidence checks shared definitions, actual placed BRep
  bounds, a source-height edit reaching both copies, history/reopen and
  noncommuting transform composition. Cyclic parenting refuses atomically.
- Native Browser rows, exact instance clicks and visible highlight are pending
  final acceptance. Independent review found legacy modeling entrypoint bypasses;
  these must refuse ambiguous instance edits before closure.
- `tests/linked_subtree_api.py` is the real-app workflow gate for shared source,
  placement, selection immutability, edit/history/reopen and explicit export
  refusal. It is not yet a passing receipt.

External evidence: `deliverables/linked-subtree-2026-10-14/`.

Joint-containing copy remains unsupported until ASM-016/#365 remaps the internal
joint and verifies the second piston loop. Occurrence-aware STEP/STL is tracked
in ASM-018/#368; current copied-design export must explicitly refuse rather than
silently omit copies or their placement. These follow-ups remain dependencies of
the complete piston tutorial gate.
