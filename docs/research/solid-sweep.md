# Retained circular Single Path Sweep — SOL-032

Public contract ID: sweep-paper-clip-warwick-2017-20261013. Exact geometry,
source images/URLs, independent transcription and unknowns are external. The
bounded foundation implements one circular disk profile swept along one earlier
retained planar open G1 line/arc chain: Full, Perpendicular, New Body. Native selection controls are added by SOL-033; source sketch-authoring and
the complete tutorial remain separate. Partial extents, noncircular sections,
Parallel, guide rails, taper/twist and Boolean operations remain broader #57
scope.

## Shared commands and authored data

`solid.sweep` creates or edits with `profileSketchId`, `profileId`, `pathSketchId`,
ordered `pathEntityIds`, and `pathStartPointId`. Optional `extent:"full"`,
`orientation:"perpendicular"`, `operation:"newBody"`, `name` and `featureId` are
supported. Unsupported enum values are refused. Existing edits retain feature,
body and ownership identities. New commands use the existing active-owner helper;
cross-component references remain refused by document dependency validation.

`command.begin({command:"solid.sweep",inputs:…})` reserves stable feature/body
UUIDs. `command.update`, `command.preview`, `command.commit` and `command.cancel`
use that same authority. `feature.editDialog` begins an API-only retained edit;
`solid.sweepDialog` and `feature.editDialog` now expose the same dock with
`nativeControlsAvailable:true`. Unsupported options are absent. Preview is actual OCCT GPU
geometry, with no authoring/history entry. Cancellation removes presentation and
session data. A rejected evaluation retains accepted document/history state.

The feature stores source IDs and ordered entity IDs, never a frozen wire or
cached mesh. Exact solved `EvaluatedSketch.entityEdges` maps source UUIDs to edges
created during the existing evaluator pass. Current solved world endpoints,
entities and support frames supply orientation; shared/transitive Coincident
identities and numerically exact contacts establish connectivity. The retained
start determines forward/reverse traversal independently of entity-array order.
There is no approximate proximity selection or implicit endpoint closure.

## Kernel and lineage

OCCT [BRepOffsetAPI_MakePipe](https://www.occt3d.com/dev/doc/refman/html/class_b_rep_offset_a_p_i___make_pipe.html)
with CorrectedFrenet transports the circular section along the oriented G1 wire.
Circle roll is immaterial; this does not establish arbitrary-profile transport.
A circle's center must meet the retained path start; the profile plane normal may
be parallel or antiparallel to its initial tangent. Unsupported, missing,
branched, disconnected, misaligned and non-G1 inputs are refused with feature
and retained-source context. The output must be one valid closed positive-volume
solid; OCCT `BRepAlgoAPI_Check` additionally checks geometric self-interference.

Internal numerical policies are explicit Alloy choices: coordinate roundoff uses
64×double epsilon×coordinate scale, tangent/normal dot checks use1−1e−10 and
profile-center alignment uses1e−7 mm. These are not documented reference closure
tolerances and do not heal path gaps. A positively separated approximately 1e-8 mm (10 pm) endpoint pair
is refused, while an explicit Coincident correction is accepted.

Only unique planar result faces found by exact identity in the pipe builder's
FirstShape/LastShape are named `sweep.start`/`sweep.end`. Stable cap UUIDs derive
from feature and role; right-handed frames use outward endpoint tangents. Caps
have no retained projected boundary edges. Curved supports, general side-face or
solid-edge naming/projection are not claimed. Topology copies protect evaluated
input edges/profile from candidate construction.

Geometry inspection exposes ordered/oriented references, path length, section
radius, extent/orientation, start/end world coordinates and cap descriptors.
See [candidate validation](../validation/solid-sweep.md).

## SOL-033 native selection contract

Create > Sweep (and its original Alloy ribbon icon) opens Profile then Path
roles. Profile requires a circular disk in the active component. Path picking
uses ordinary authored finished-sketch lines/arcs in their current resolved
frames, independently of AIS current selection. Source sketches are temporarily
available during the command, including auto-hidden retained sketches during
edit. Their authored visibility is unchanged by preview or Cancel.

`solid.sweepSelection` takes optional `role:"profile"|"path"`, `clear:true` and
`chainSelection:boolean`. `selection.pick({x,y})` uses the same selector as
viewport mouse events. Clearing Profile clears dependent Path references;
clearing Path retains Profile. Successful profile picking activates Path.
Selector results expose picked entity/profile IDs and complete retained inputs.

Chain Selection is checked by Alloy choice. With it checked, selecting a section
expands the connected ordinary line/arc component; it must be open and
unbranched, and the shared Sweep evaluator requires G1 continuity. A branch is
refused rather than choosing a route. Disconnected/closed selections are not
healed. With it unchecked, individual contiguous sections are added from the
section meeting the circle center. Repeated selected-section clicks are
idempotent. Toggling preserves existing selection and changes subsequent picks.
The endpoint meeting the profile center determines traversal, independent of
source-array or click order. The command retains ordered entity IDs and start ID.
This is a bounded Alloy policy, not an undocumented exact propagation/default
claim. The public screenshot establishes a checked control in that state only.

Full Distance1.00, Perpendicular and New Body are readable fixed values, with
no unavailable selectors disguised as options. Name, Profile/Path roles and
Chain Selection are active controls. Blue circular profile and orange topmost
selected edges remain readable over actual gray solid preview. Errors are
visible and disable native OK until correction; no rejected geometry is
committed. Cancel/OK/Escape and Timeline editing use shared authority. Selection
highlights and temporary source visibility are removed when the session ends.
The chain checkbox is command-selection policy, not a new authored geometry
parameter; reopening an edit uses the stated checked Alloy default.

Source exact sketch authoring, typed circle/key parity, full cumulative tutorial
and general Sweep controls remain separate gates. Public screenshots are older
and unmatched in OS/build/DPR, so exact visual fidelity remains unqualified.
