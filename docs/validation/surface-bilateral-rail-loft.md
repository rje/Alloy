# Retained bilateral rail exterior and handle openings

SRF-017/#361 independently accepted2026-10-15 at `fc3ba9c`. The bounded Rails
Loft now accepts one back arc, a retained Mirror pair of ordinary open fit
splines and a middle-plane rail meeting the arc interior and common lower tip.
The existing endpoint-contact arc/two-line mode remains supported. Inputs come
from actual solved sketch geometry and retained Mirror ancestry; no fixture
coordinates or approximate substitute surfaces are used.

Strict source-local symmetry, planarity, unique contact and finite parameter
checks admit an exact tensor construction using OCCT degree elevation and knot
insertion. Two faces share one smooth support. Their IDs derive from lower
source entities; five ordinary edge IDs derive from the rail, lower entities
and split arc spans. Boundary resolution uses unique sampled correspondence
against exact constructed restrictions, not OCCT builder history or nearest
candidate matching. Two collapsed-tip artifacts remain valid topology but are
excluded from selectable edge maps, also in downstream Trim. Regularity checks
are sampled and exclude the known collapsed boundary; global injectivity is not
proven. Other source networks remain unsupported.

The actual retained chain passes source sketches→Loft→paired-sketch Trim with
two valid closed two-edge side openings. Crown edits preserve identities;
undo/redo, exact authored reopen, arbitrary rigid-frame invariance and atomic
contact/ancestry refusal pass. Existing rail Loft/Trim regressions pass2/2.
The43-call real-app flow covers preview/Cancel/commit, both region picks, source
edit/history/reopen. Qt native3/3 covers actual curve picks, preview/Cancel and
OK. Apple M4 hardware framebuffer captures and resize evidence were inspected.

Initial GPU captures exposed alpha-ordering artifacts despite correct mesh
normals. Committed surfaces now honor authored opacity1.0 instead of forced12%
transparency; explicit lower opacity remains intact. Surface Loft uses the
existing opaque neutral preview path. Smooth final native and both side views
pass a bounded14-call saved-model replay, without repeating unchanged geometry.
No mesh tolerance, geometric input or global transparency mode changed. Exact
reference preview styling remains unknown. REL-027 preserves explicit
translucency quality; REL-026 owns measured13–28second Trim interaction latency.
This is functional/visual acceptance, not responsiveness or release acceptance.

External evidence: `deliverables/surface-iron-continuation-2026-10-15/retained-01/`
contains README, source/app hashes, core models/log, API result/transcript/model,
native-final controls/captures and render-final clean models/GPU capture set.
The adjacent exact-tensor prototype records algebra and source admission.
Astra independently reviewed construction, source, core/API/native evidence and
final captures; root inspected the smooth isometric side-hole capture.

The independently authored prepared sketch dimensions and contact arrangement
are qualified choices. Public video observations confirm2profiles and1rail;
exact source coordinates/contacts remain unknown. The two openings supply the
next SRF-015 three-section bridge. Its actual contacts, subsequent crossing-sheet
trials and final handle-bearing Boundary Fill cell remain in SRF-015/SRF-016/
SRF-011 and cumulative TUT-018; this milestone does not close that tutorial.
