# Source-sized opposed snap group: focused geometry gate

The adapted Top cantilever uses the retained three selected Join cells (20+7+1 mm²), Chamfer 2 mm, Chamfer 1 mm, the current wall/stem attachment-root Chamfer 2 mm, and an inner hook/stem elbow Fillet 0.5 mm. The elbow is the straight Y edge at X−28.5, Z12.5, Y17.5–22.5. Its selection is a disclosed fit inference: the public transcript names the inner snap corner and values, but does not show an unambiguous edge click in the available record. No alternate edge or reduced finish value was used to obtain the pass.

The focused source fixture is external `../Alloy-reference-records/deliverables/enclosure-snap-2026-10-14/root-chamfer-probe-01/exact-root-inner-fillet.alloy`. `snap_union_mirror_test` adds the opposite Bottom catch by retained YZ Cut Mirror, then mirrors the five Top features across YZ. This is the explicit coordinate adaptation from the documented XZ source plane. The test proves preview/commit, connected Top, exact source-root input and 2 mm authoring, Join 5→5.25 edit, undo/redo, and save/reopen. Its final authored model is external `../Alloy-reference-records/deliverables/enclosure-snap-2026-10-14/root-chamfer-probe-01/exact-two-mirrors.alloy`. The reflected attachment root is published before the replayed Chamfer consumes it; expecting that edge to remain current after the full group would be incorrect.

At this intermediate source order, the Top inner-elbow Fillet intersects the still-sharp Bottom catch by 0.268252 mm³ on the seed side. The mirrored side adds the same 0.268252 mm³; no other body gains positive-volume overlap. The common occupies the two inner elbows at Z12.5–13. The public workflow subsequently adds Bottom inner-cutout Fillet 0.5 mm, then reorders that feature after both Mirrors and adds the opposite edge. Therefore this intermediate file is **not** final physical-fit acceptance. The final zero-added-interference gate belongs to the reordered Bottom finish and cumulative workflow.

GEO-015's strict concavity check originally refused the reflected 2 mm root because OCCT represented one reflected plane with an indirect parameter frame. Using the oriented U×V surface normal preserves the original classifier and all endpoint/leg/volume guards; the corrected source-sized five-feature Mirror passes. The earlier technical smaller-finish root-identity gate remains distinct from this source-size result.

Astra independently accepted this bounded ingredient on 2026-10-14 at
`b92db08`. The durable focused output is external
`enclosure-snap-2026-10-14/source-group-final/focused-core-pass.log`, ending
`MIRROR_COMMITTED 28 exactSourceRoot=1`. The subsequent accepted
[Timeline reorder gate](timeline-reorder.md) verifies both separate Bottom
R0.5 groups and zero final Top/Bottom positive-volume intersection.
