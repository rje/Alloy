# Ruled-wall corner construction

2026-10-15. GEO-019 is independently accepted for the bounded grouped R12/R16
corner construction. The full shoehorn remains open.

The exact 3 mm thickened source now creates one grouped R12/R16 corner feature
with a valid closed positive solid and all four generated round supports. A
bounded fallback uses the retained Thicken's four ruled sidewalls, exactly
extends their linear thickness parameter, constructs independent rolling-ball
corner cutters and subtracts them from an equivalent shared-support source copy.
The original source trims and geometry are preserved. Operation order follows
input topology, independently of generated UUIDs; persistent IDs still derive
from source ancestry. Two equivalent saved models now produce identical round
supports and matching adaptive volume 17,033.693449192317 mm³. This corrected an
observed UUID-dependent geometry difference before acceptance. Production admission uses
retained wall/seed ancestry and a unique four-wall cycle, not fixture dimensions
or transient face numbers. Ordinary successful fillets retain their existing path.

The result must retain all six original supports and all four generated round
supports, with no auxiliary cap surface or material outside the source. Validation
Booleans are non-destructive. Ambiguous topology or invalid construction refuses
without replacing the committed model. The source may legitimately produce one,
two or three physical edges along a round/broad-face boundary; all are preserved.
Eight logical boundary chains derive from corner and broad-face ancestry.

A length change from 130 to 140 mm retains the four round-face IDs and eight
logical chain IDs. Physical edge topology changes from twelve to fourteen named
round-boundary segments. A whole edge and a split terminal segment have distinct
IDs: reusing an ID would silently change its other endpoint. Tangent-plane point
references admit only endpoints anchored to a named sidewall; numerical interior
trim-fragment vertices are not stable modeling references. A retained terminal
on an unchanged segment passes the positive edit gate.

The bend change from 16 to 18 mm currently refuses atomically because one
required corner support is absent after the combined Cut. This is a known
supported-domain limit, not successful edit coverage. It is not retried with
changed radii or a generic repair sequence.

Focused core evidence covers geometry, history/reopen, the supported length
edit, retained tangent plane and refusal. Evidence lives externally under
`shoehorn-workflow-2026-10-15/geo019-product-api-01/`: `core-ordered.log`,
`volume-probe-ordered.log` and the original 77-call API receipt. The corrected 21-call app replay and both final captures under `ordered-api/`
are independently accepted; source-length edit, exact authored undo/redo and
reopen pass. Astra reviewed source, core receipt, API data and actual captures. Hardware GPU metadata reports Apple
M4, OpenGL 4.1 Metal-90.5 and DPR 2. Native-framebuffer captures are app visual
evidence, not manual pointer/keyboard acceptance; the command flow here uses API. The separate R1.5 perimeter construction is GEO-022; generated
references through that operation remain GEO-018. The actual final hole,
organization, complete creation movie and checked printing file remain TUT-017.
