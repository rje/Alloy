# Bounded solid-edge fillet candidate

SOL-022 / #182 implements one constant-radius G1 rolling-ball selection set,
with Tangent Chain **On**. Off, radius dragging and broader fillet modes remain
in SOL-003 / #48. The source-video gate uses the two outer-prong vertical corners at inferred source coordinates
(32,-10) and (32,0), with radius 1 mm. Public selection/result frames identify
both as side corners; the earlier top-rim interpretation was corrected by
independent review of the final planar-cap preview. The separate PDF six-edge / 0.5-mm variant is unclaimed. The external
contract is `knowledge/specs/solid-fillet-support-2026-10-12.md` in the reference
records sibling; world-coordinate identification of the two video picks is
explicitly inferred from the authored source fixture.

UI Modify > Fillet and F open an edge-selection dock with zero initial radius.
Click actual edges, enter a positive millimeter radius, inspect the disposable
neutral-solid preview with readable blue edge cues, and choose OK or Cancel.
OK is enabled only for a current valid preview. Escape cancels from either
panel or viewport. Feature history opens the same dock for editing.

`solid.fillet` authors `targetBodyId`, `edgeIds`, `radiusMm` and
`tangentChain:true`; `featureId` edits an existing feature. `solid.filletDialog`
and `command.begin` open the shared session. `selection.pick` selects real
provenance edges, and `view.projectEdge` projects their actual midpoint for
inspection. Selection hit testing uses screen proximity and face occlusion;
persistent resolution uses only authored provenance and OCCT shape history.
No edge ordinal or nearest-coordinate identity is stored.

Prism end rims use generator/profile/source-boundary contact signatures;
side corners use the same generated vertex/contact lineage. Exact surviving
or unique Boolean Modified descendants carry identity through Join. Deleted
or ambiguous references retain explicit unavailable descriptors. Duplicate
records are aggregated before resolution, independent of participant order;
an unavailable descriptor in another output cannot hide a live merged edge.
Established ambiguity cannot be cleared by a later live duplicate.

Fillet uses OCCT tangent contours. Seed IDs and evaluated contour descriptions
are exposed separately. Invalid or impossible radii refuse atomically. An
unaffected original disk cap survives for the next supported sketch; arbitrary
filleted-face attachment and general topology repair remain parent scope.

## Verification status

Private Qt 6.11.2 / OCCT 7.9.3 application builds. `solid_fillet_test` passes
actual prism provenance, the two 1-mm seed fillets, radius/height edits, atomic
impossible-radius refusal, deleted-reference diagnostics and merged descriptors
in both participant orders with retained ambiguity. The same focused test takes
an optional source-order joined `.alloy` fixture to check the complete source
fillet, retained disk-cap sketch, edit, undo/redo and exact reopen.

`tests/solid_fillet_api.py` checks the saved joined source fixture through the
actual app: edge hit testing, preview without authored mutation, Cancel,
commit, edit, undo/redo and save/reopen. The joined source core passes against the #176 fixture: two actual seeds at
1 mm, radius edit to 0.5 mm, outer height 24→26 retaining edge IDs, one
undo/redo, original disk-cap20 supported Ø5.2 circle, impossible-radius atomic
refusal and exact save/reopen. Run:

```sh
./build-fillet/solid_fillet_test ../successive-join/build-successive/evidence/support-successive.alloy
```

Integrated real-app/API verification passes at
`build-tutorial-polish/evidence/solid-fillet/report.json`, with preview and committed
captures. Startup refresh now reads session state without inserting an empty
command; radius schema explicitly permits the dock range. These two integration
defects were caught and corrected before native review.
Independent native F/two physical picks/capture evidence remains pending.
No exact native or visual match is claimed by the build/core evidence alone.
