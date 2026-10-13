# Face-positioned Sphere controls candidate

SOL-014/#145, 2026-10-13. Source contract: `tutorial-library-die`, recorded
externally in `knowledge/tutorials/warwick/library-die-ledger.md`.
See [the core contract](../research/face-sphere.md). Public instructions specify
an actual planar body face, a center click and diameter, an intersecting red Cut
preview, OK/Enter and later right-click Repeat Sphere. Numeric fixture values
are Alloy choices, and exact Repeat parameter carry-over is unknown in the source.

## Shared implementation contract

`solid.sphere` and `solid.sphereDialog` use `support` (the exact body/producer/face
triple), canonical face-local `uMm`/`vMm`, `diameterMm`, `operation` (`cut` or
`newBody`), Cut `targetBodyId`, optional `featureId` and `name`. A face-positioned
command defaults to Cut on that face's body. Initial diameter 3 mm and carrying
forward the last accepted diameter are explicit Alloy fixture/interaction choices.
The API exposes metadata, session inputs, selection role, evaluated geometry,
structured failures and completion through the existing shared command authority.

The Create Sphere action opens a dock before selection. First viewport click
picks the actual retained planar face; a second ray maps to its face-local center
without forcing Look At or entering a sketch. Hover highlighting is reused.
Alternatively an already selected face binds before center placement.
`selection.faceSet` and `selection.facePick` invoke the same face-selection path.
`view.projectFacePoint` projects an exact support and local millimeters into
logical viewport pixels for inspection/automation. Selection role is `face`,
`center` or `placed`, independent from authored modeling state.

The panel reports face and center, edits diameter, exposes Cut and New Body,
previews the actual sphere tool (translucent red for Cut, blue for New Body) over
retained source stock, and offers OK/Cancel. Center placement focuses/selects the
diameter field. Enter interprets pending typed diameter and commits; Escape
cancels from field or viewport. New Body and feature IDs are reserved once per
session. Timeline feature editing evaluates the prefix support and validates the
whole downstream document. Edits retain operation/ownership. Failure preserves
usable authored geometry/history.

The nonmodal viewport context menu has named `context.repeatSphere` on an actual
clicked supported planar face. It invokes `solid.repeatSphere {support}`, clears
the previous center and binds the new face while retaining the last accepted
diameter. It does not fallback to an older selected face, implement a radial menu,
or claim a general repeat-last-command framework. Public radial presentation
remains an interaction mismatch. Placement on an existing pip opening is refused.

## Evidence and remaining qualification

The application and focused `sphere_controls_test` compile against the isolated
coherent Qt runtime. The existing core `solid_sphere` CTest passes (six face IDs,
21 real hemispherical cuts and volume, upstream rounding edit/history/reopen/New
Body). No kernel behavior was altered by this controls candidate.

Prepared, not yet run:

- `tests/sphere_controls_api.py`: real app face-pick/center rays, red preview and
  Cancel, retained Cut, feature edit preview/commit/cancel, Repeat center clearing,
  impossible placement, stable New Body preview identity, undo and exact reopen.
- `sphere_controls_test`: focused Qt mouse events for face and center, typed
  diameter/Enter, real nonmodal context action and Repeat/Escape. This is synthesized
  Qt input against the actual app/GPU, distinct from physical OS input.

Independent native/visual acceptance remains pending. No matched public capture
is available for exact pixels. The full die/component/STL/movie gate remains a
separate cumulative deliverable. No issue closure, release or other-platform
runtime qualification is implied here.
