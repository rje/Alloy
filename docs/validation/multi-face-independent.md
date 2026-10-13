# Independent multi-face child acceptance

2026-10-13. Independently accept SKT-036/#212 on aa815a2 and SKT-035/#209 on
69c98a3. This does **not** accept parent SKT-017/#150. Native recovery remains
SKT-037/#213 Todo; broad #159/#160/#161 remain separate. The reviewer inspected
final source, tests, authored files, API report and actual captures without
replaying unchanged workflows or opening another editor.

For #212, saved native models contain retained support triples, independently
specified horizontal/vertical dimensions8×3 and6×3, and Join4/2 against the
correct original body IDs. Recorded volumes4954.62833058846 and3036mm³ verify
added volumes96 and36. Evaluated side and inclined frames are right-handed
(cross(u,v) dot normal =1); the oblique support comes from the XZ trapezoid
prism, with no Move substitute. Qt test source and recorded artifacts establish
face click→Create Sketch→pointer rectangle→D/line/placement/typed dimensions→
Finish→profile pick→Join/target controls→distance spin arrows→OK. API-seeded
stock is explicit; this is synthesized Qt input, not physical OS operation.
Join text/Return did not commit in the inactive test window and is unqualified.

The retained API report separately passes width40→50, hole center35→45, side
boss retention after an earlier width edit, preview Cancel, undo/redo and exact
reopen. Changing the trapezoid slope vertex20→24 changes the attached world
frame and points while retaining support identity; final volume3436mm³ retains
the36mm³ boss. The reviewer inspected the saved equations and these evaluated
results. Oblique joined captures show real outward boss depth; head-on previews
show readable controls but do not independently convey depth.

For #209, the production catch preserves the original structured diagnostic,
including code/message and body/producer/face. It fills missing feature identity
with the affected sketch and preserves any existing upstream attribution. Failed
evaluation remains detached and returns before document apply/history push.
Final core assertions cover both split and deleted support with sketch ID/type,
full support triple, and unchanged authored state/revision/history count.
The actual API report and red error capture independently show the same
support_unavailable cause. API Cancel/correction preserves accepted state.
No native recovery success is inferred from that API result.

The final native recovery probe failed to toggle the intended tool checkbox,
leaving the original valid tool set. It therefore proves neither a modeling
failure nor successful native refusal/Cancel/correction. #213 must resolve that
input route and finish visible recovery evidence before #150 closes. Relink or
replacement-support UI is not implemented or claimed.

Visual limits remain explicit: native immediate captures have blank Timeline
strips and coincident tile bounds, unlike settled API captures; deferred layout
is suspected but unproven. Sketch summary clipping was independently corrected
and accepted under #211. Narrow toolbar truncation remains separate. No exact
visual parity, general topology repair, cross-platform runtime or full native
cumulative-gate acceptance follows from these bounded children.

Evidence is in the isolated candidate's `build-multi-face/evidence/{api,widget}`;
see [the candidate receipt](multi-face-gate.md) for commands and individual files.
