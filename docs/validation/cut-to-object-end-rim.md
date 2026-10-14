# Exact end rims for straight Cut To Object

GEO-012 extends the existing straight-Cut Boolean history transfer to prism end rims. A current straight end edge is published only when OCCT traces its tool edge to one retained planar target face and exactly one source contributor. Its stable ID uses the Cut feature, source sketch entity, end role, and target face; existing outer Cut rim IDs stay unchanged. Missing, merged, or ambiguous history remains unavailable.

The real-app proof opened the accepted split Case, sketched a 6 × 3 mm closed catch on the retained Bottom side at a guide endpoint 4 mm below its top, and Cut To Object to the retained inner wall. The Cut removed 63 mm³. A separate Cut-feature Mirror across XZ removed another 63 mm³. On reopening that authored model, each catch had four current inner `cutEndRim` IDs on the inner wall in addition to its four outer rims. Evidence is under `../Alloy-reference-records/deliverables/enclosure-snap-2026-10-13/undercut-probe-02/edge-inspect-mirrored/` and the source fixture/ledger under `undercut-probe-01/`.

The two distinct upper inner IDs accepted a 0.5 mm Fillet: the first edge created the feature and editing that same feature added its mirrored edge. The resulting solid stayed valid and closed after save/reopen. Editing the authored catch width from 6 to 5.8 mm then replayed both Cuts, Mirror, and Fillet; undo/redo and reopen kept the authored Fillet edge IDs and valid solid. The Fillet consumes those edges, so they are unavailable in the final post-Fillet edge catalog. Focused receipts are `undercut-probe-02/fillet-pair/summary.json`, `width-edit/summary.json`, and `bottom-two-catches-left.png` in the external directory. The existing `cut_open_rectangle_rim` core regression passed.

The horizontal y=±20 catch placement is an adaptation to this independently authored input. This proof does not claim the later joined cantilever group, final feature order, physical snap strength, or native selection of the new inner edge.

Astra independently accepted this exact-lineage ingredient. Main application
build and the existing open-card-rim regression pass. The provisional −X
seed mirrored across XZ yields two catches on the same wall; it does not
qualify the tutorial's opposed-wall closure. That cumulative axis/placement
question remains TUT-022 and must be resolved before its acceptance.
