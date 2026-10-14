# Retained circular model-edge Project candidate

SKT-071 candidate, 2026-10-13, isolated `feat/circular-model-project` from
`fb2d315`. Public enclosure workflow and adapted fixture provenance are external
under `../Alloy-reference-records/knowledge/tutorials/advanced-audit-2026-10-13/enclosure-prefix-contract.md`
and `../Alloy-reference-records/deliverables/enclosure-adapted-input-v2-2026-10-13/`.

Project accepts a true analytic circular edge on an imported STEP occurrence
when its plane is parallel to the supported sketch face. The authored sketch
stores only the edge and occurrence identity. Evaluation recomputes the center
and radius from the current OCCT edge after the captured outer placement;
solver-locked virtual geometry is removed from the authored sketch. A purple,
read-only projected circle can bound a profile and drive an ordinary retained
Circle Offset. Ellipses, nonparallel circular edges and unavailable sources
are refused; general curved Project and relink/repair remain in SKT-026.

The focused core test uses the qualified v2 PCB fixture, four distinct top
mounting edges, an identity-sibling Case face at z3.5, four projected radius1.35
circles, four Offset−0.3 results of radius1.05, and Join5 pegs. It checks exact
transformed centers, captured placement following, undo, save/reopen, and atomic
missing-source refusal. The previous straight-edge linked Project test also
passes. The 43-call real-app API run repeats four holes, offsets, Join, capture update,
undo and reopen through the public control API. A native Qt viewport click
picked one actual PCB circular edge after `P`, showed a linked preview, and
Escape restored the unchanged sketch. Input was Qt-delivered; no OS physical
pointer or source-product runtime was used.

Evidence: external `../Alloy-reference-records/deliverables/linked-circular-project-2026-10-13/api-02/`
contains the command ledger, saved Alloy file and inspected GPU captures;
`native-04/` contains the passed native Project preview and panel captures.
The four offset curves are small at the current full-board view scale. This
bounded ingredient is not the full case-shell/analysis/peg creation movie;
TUT-020 owns that cumulative gate.
