# Retained rectangular Body Pattern candidate

SOL-085 / #623 adds a bounded exact body-pattern core and shared local control
lifecycle. Native Bodies controls, command-first body/edge picking, native
Timeline edit and 1×/2× GPU visual acceptance remain SOL-092 / #654. This receipt
does not establish native interaction or screenshot parity.

## Contract

`solid.rectangularPattern` dispatches to body mode when `sourceBodyId` is
explicit. Use one earlier active root solid and `axes`, a one/two-element array
of `{edgeId, quantity, spacingMm}`. Directions are retained straight edges of
that source body. Their retained endpoint orientation is used when available;
other named straight edges use their current kernel parameter orientation.
Two directions may be oblique; parallel/antiparallel pairs refuse. Curves,
missing edges and non-root bodies refuse.

Quantity is an integer 1..128 including the seed; at most 128 total placements
are supported. Spacing is finite, positive and at most 100000 mm. One direction
makes a row; two make a grid. Counts including the seed, endpoint orientation,
positive-spacing restriction and limits are Alloy
policies, not public-source default claims. There is no implicit direction or
spacing default. Axis 2 quantity 1 also gives a single row.

The authored feature type is `rectangularBodyPattern`, separate from existing
additive/Hole `rectangularPattern` storage. It retains the source ID and axes;
no copied BRep or generated occurrence rows are authored. Generated body/face/
edge UUIDs derive from feature identity and coordinate `(i,j)`, so changing the
first count does not rename surviving second-row placements. Seed geometry,
identity, ownership and summary remain unchanged. Exact evaluated copies use
`SolidTransform::copy(..., false)` and retain independent selection/support
identities. Source edits recompute all placements; downstream references to
removed placements refuse atomically.

`command.begin` with `command: solid.rectangularPattern` and explicit body inputs,
`command.update`, `command.preview`, `command.cancel` and `command.commit` share
the same candidate/evaluator/commit authority as the direct command.
`feature.editDialog` restores retained body inputs to this shared API session.
Body mode currently opens no native panel; its preview reports all occurrences
and displays a transient exact copy compound. The prior feature-pattern panel
continues to serve additive/Hole mode. Native body selection/presentation is the
linked child, not an implemented claim here.

## Evidence

Validated locally on macOS arm64 with Qt 6.11.2, OCCT 7.9.3 and AppleClang 17 in
`build-preview-release` (Release), built with `-j4`; no CI dispatch or tag push.

- `rectangular_body_pattern` core: asymmetric 12 × 3 × 7 source; row/grid exact
  volume and centroid placement; independent topology; unchanged seed; stable
  coordinate identities across first-count extension; source-height recompute;
  downstream generated-body Move; nonparallel oblique-grid vector-sum centroid;
  missing/parallel directions, non-root ownership and invalid/non-finite
  quantity/spacing refusal; removed referenced occurrence refusal; edit
  Undo/Redo and authored save/reopen.
- Existing `feature_pattern` core regression passes unchanged.
- `tests/rectangular_body_pattern_api.py` runs through the real application
  entrypoint/local server (53 calls): preview/Cancel, one direct row commit, retained grid
  edit/commit, independent committed body selection, source edit, refusal,
  Undo/Redo and save/reopen; oblique-grid exact placed bounds and curved
  cylinder-edge refusal. Independent binary STL readback checks eight
  disconnected watertight components, placed bounds and each actual volume.

External actual-app evidence and saved model/export:
`../Alloy-reference-records/validation/rectangular-body-pattern-2026-10-20/api/`.
The process is closed by the harness. Console lock precludes native/physical
input and human-visible GPU acceptance; API success is recorded separately.

Research contract and public provenance live outside the repository at
`../Alloy-reference-records/knowledge/specs/sol085-body-rect-pattern-2026-10-20.md`.
Documented public inputs/grid behavior are implemented for this bounded edge
family; exact defaults, styling, keys, preview and native control parity remain
unknown or deferred. Other reference families, feature/
component modes, Extent, symmetric distribution and suppression remain outside
this slice.
