# Shortened straight rim lineage — candidate (#245)

Status: bounded DOC-020 candidate, 2026-10-13; independent review pending.
Public basis: `plate-countersink-pattern-20261013`, page 3, retrieved 2026-10-13.
The public practice shows a centered 75 × 50 × 10 mm plate, four vertical R5
Fillets, then a top-face sketch point 10 mm from the adjacent top and left rims.
Detailed public provenance, drawing and command unknowns remain external in
`knowledge/tutorials/next-after-outlet/independent-countersink-contract.md`.
The standalone Point tool and the complete countersunk plate remain separate
children; the fixture here uses an existing circle center to exercise the
accepted point-to-edge dimension foundation.

## Implementation contract

The surviving planar support and each retained rim keep their source identities.
One actual result edge must be either the exact surviving OCCT edge or the
unique `Modified` descendant reported by the operation. No coordinates choose
replacement geometry, generated arc or topology index. Fillet opts into changed
straight-rim retention; other planar-face transfer callers keep their previous
conservative policy. A mapped rim must remain a nondegenerate, collinear straight
edge. Coordinates verify that chosen descendant and preserve its prior direction;
they do not search for one. Split or missing support boundaries are withheld, so
existing authored projections report `projection_unavailable`. Named solid-edge
references retain an unavailable descriptor with `ambiguous_split` or `deleted`.
Modified curved edges remain unavailable in this slice.

Changed rims expose their current actual endpoints with
`endpointProvenance=currentKernelBoundary` and `lineage=uniqueModifiedStraight`.
Old `startContact`/`endContact` fields become `sourceBoundaryContacts`, explicitly
ancestry rather than claims that the shortened ends are old sketch corners.
Projected endpoint UUIDs identify the current start/end roles of the retained
line, not the original authored corner point UUIDs. Source direction preserves
those roles even if OCCT reverses its curve parameter direction. Subsequent
Boolean transfer refreshes this endpoint metadata for a straight descendant,
or clears it before reporting another curve's endpoints.

The four current `endRim` solid-edge IDs are available to model-edge selection
and direction discovery. **Direction-consumer requirement:** a consumer defining
signed direction from start/end must use the oriented descriptor endpoints.
The existing additive Pattern evaluator uses raw curve parameter direction;
its future #248 integration must reconcile that path before claiming agreement
with the displayed endpoint signs. This candidate establishes current edge
identity and projection, not new Pattern behavior.

## Verification and evidence

Isolated build: `Alloy-worktrees/fillet-rim-lineage`, base `893679c`.
Evidence directory: `build/evidence/fillet-rim-lineage/`.

- Debug application and affected core targets compiled against Qt 6.11.2 and
  OCCT 7.9.3. `build.log` records the final production build.
- Final focused core passed 3/3 in 1.39 s: `fillet_rim_lineage`, `solid_fillet`
  and `point_edge_dimensions`. New coverage checks all four support edges
  against actual face topology/current curve endpoints, all four named top
  solid edges, analytic rounded volume, two retained 10 mm point distances,
  centered width 75→85, radius 5→4, unchanged authored equations/projections,
  exact undo/redo/reopen, and atomic impossible-radius/unavailable-ID refusal.
- Explicit history controls split one boundary into two actual candidate edges
  or leave it missing: no candidate is chosen, direction IDs become unavailable
  and an existing projection fails explicitly. A reversed straight curve
  preserves endpoint roles. A later unique Boolean descendant refreshes current
  endpoint metadata rather than exposing the prior longer segment.
- `tests/fillet_rim_lineage_api.py` passed 59 shared-command calls through the
  actual native executable. It writes `api/report.json`, `api/transcript.json`,
  `api/rim-dimensions.alloy` and three actual GPU captures. Both dimensions retain
  10 mm; the circle center moves from (−27.5,15,10) to (−32.5,15,10) after the
  centered width edit. Radius edit, exact history/reopen and both invalid input
  refusals pass; `view.projectEdge` resolves a current shortened named rim.
- Inspected `two-edge-dimensions.png`: rounded real plate, current projected
  straight boundaries, two readable 10 witnesses and separately labeled R1
  circle. `reopened-edited-dimensions.png` records the larger edited plate.
  `appState.viewport.gpu` reports Apple vendor / Apple M4 / OpenGL 4.1 Metal,
  hardware acceleration and 2376 × 1226 native framebuffer at 2× scale.
  The private harness instance exited; no user-owned editor was operated.

API evidence precedes the final opt-in routing and later-Boolean metadata guard.
The fillet branch used by that fixture is unchanged. The final build and core
checks cover those small final changes; unchanged API behavior was not replayed.
`source-fingerprints.json` records reconstructed pre-routing and final production
source fingerprints and the precise evidence boundary. Core, API and screenshot
claims remain separate. No physical-input, matched pixel styling, standalone
Point, Hole, subtractive Pattern, other-platform runtime, release or complete
tutorial acceptance follows from this prerequisite.
