# Retained section support after Split

DOC-032/#357 follows a face-linked section through authored Split descendants.
The original stored producer, body and face reference remains unchanged. Only
bodies reached through the original body's ordered Split history are searched;
the current face must retain the exact producer and face IDs and pass the
existing real planar-face validation. There must be exactly one match. No
coordinate or nearest-plane fallback is used. A face divided between two halves
is ambiguous and leaves the section unavailable, rather than choosing a half.

The actual saved enclosure Top section resolves onto the new Top half and clips
the GPU scene at z=24.5 mm with normal −Z. Changing the original Case extrusion
from 30 to 31 mm moves that same section to z=25.5; undo/redo and save/reopen
preserve the authored support. A separate box side face bisected between both
halves refuses ambiguity and restores after undo. The 31-call application run
and inspected screenshot pass at external
`../Alloy-reference-records/deliverables/section-split-support-2026-10-14/api-01/`.
Build and actual-app API/GPU evidence are distinct from physical input; no new
native controls were introduced. Independent source, transcript and GPU-capture review accepted the bounded change.

The previous enclosure recording and its origin-plane workaround remain
historical evidence. This follow-up fixes the original retained analysis in the
saved model; it does not revise old footage or qualify the alignment rim.
