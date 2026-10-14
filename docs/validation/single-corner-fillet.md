# Single-junction sketch Fillet — SKT-087

Fillet now accepts one picked normal-line junction: the existing preview appears,
radius entry receives focus, and Enter commits the retained tangent arc. A second
junction click still collects the common-radius pair; adjacent-side selection is
unchanged. Shared command acceptance, radius editing, prompts and discovery text
now describe the one-or-two-corner route. The geometry kernel is unchanged.

The focused actual-Qt control check passes3/3 in the external
`deliverables/single-corner-fillet-2026-10-14-native-01.log`: one corner previews
without authored mutation, Escape restores the prefix, and a subsequent two-corner
batch accepts twoR6 arcs through native picks and typing. The source-order
`base_sketch_tutorial` native01 passes the actual one-junction click, naturally
focused10mm entry and Enter. It verifies R10 and unchanged remote circle-contact
and Tangent records across the Fillet. Original preview/accepted captures are in
`deliverables/base-sketch-construction-2026-10-14/native-01/` outside Git.

Public printedp23 Step4 requires this single-junction route. Provenance and source
illustrations are in the external base-sketch steps1–5 contract, retrieved
2026-10-14. NativeQt evidence is distinct from physical OS input; the full tutorial
uses a nonblocking menu opener and actual submenu/action clicks. Full circle/arc
construction acceptance is TUT-025, not this UI ingredient.
