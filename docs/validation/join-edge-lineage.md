# Join and Chamfer edge lineage candidate

GEO-010/#329 supplies the retained edge history needed to author the later
five-feature snap group. Extrude Join now passes its source-stage prism edge IDs
through the existing OCCT Boolean history; only current result edges remain
selectable. Chamfer now uses the existing exact Modified history transfer so a
unique shortened descendant retains its source edge ID. Ambiguous or removed
descendants remain unavailable. No new generated-edge naming, Mirror replay or
general topology matcher is introduced.

The headless source-shaped proof starts from the accepted centered 93×64 stock,
Extrude30, grouped R5/R2.5, closed Shell3.5, Midplane and actual Split Body.
On the positive split half it authors the public 8/2/10/6/2 stepped outline,
joins it 5 mm, then commits separate Chamfer2, Chamfer1, Chamfer2 and Fillet0.5
features using ordinary retained edge IDs. The sketch placement on the outer
side face is a fixture adaptation; it does not claim the full offset-plane,
projected-cutout snap setup. Join publishes 17 current prism edges. The first
Fillet selects the original retained snap edge after the three Chamfers modify
it. Editing Join distance 5→5.5 rebuilds the finished solid; an unavailable
edge edit refuses without changing authored state. Undo/redo and save/reopen
preserve all five features and IDs. The exact post-Split source family, sizes
and operation order are exercised; final physical snap fit is a later gate.

Manual headless command:

```
build-join-edge/snap_group_proof_test <accepted-rounded-shell-split.alloy> <output.alloy>
```

The proof passed; focused `multi_face_gate`, `solid_chamfer` and `solid_fillet`
regressions passed 3/3. External evidence is under
`../Alloy-reference-records/deliverables/join-edge-lineage-2026-10-13/core-01/`
(`proof.log`, `source-snap.alloy`). The test target deliberately takes the
qualified external fixture path as an argument; CI does not depend on that
external research artifact. Native input and GPU presentation are not claimed
for this geometry-lineage prerequisite. Independent source review is pending.

Independent review accepts GEO-010/#329 as a backend prerequisite. The reviewer
ran the exact five-operation post-Split proof and three focused regressions.
Main app builds; its focused face/Chamfer integration checks pass. Full source
placement and native/API interaction remain with the cumulative snap workflow.
