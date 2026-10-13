# Retained circular Single Path Sweep — SOL-032

Public contract ID: sweep-paper-clip-warwick-2017-20261013. Exact geometry,
source images/URLs, independent transcription and unknowns are external. The
bounded foundation implements one circular disk profile swept along one earlier
retained planar open G1 line/arc chain: Full, Perpendicular, New Body. It does
not implement native controls, chain picking, source sketch-authoring gestures,
partial extents, noncircular sections, Parallel, guide rails, taper/twist or
Boolean operations. Those remain #224/#225 and broader #57 scope.

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
`nativeControlsAvailable:false` and invisible dialog state are explicit. No native
panel shell or unsupported options are presented. Preview is actual OCCT GPU
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
