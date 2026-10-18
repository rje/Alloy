# Proposed-v1 ownership audit

2026-10-17. This checks issue ownership for the 134 `proposed-v1` rows in
[the command inventory](command-inventory.json), not whether those capabilities
are usable or part of an approved release promise. Live GitHub issues and the
project board are authoritative; the repository manifest is a dated mirror.

The first pass added 20 bounded Todo owners, #496–#515. A second pass found
six gaps that were not owned by their mapped issues and added them as Todo:

| Inventory row | New owner | Missing result |
| --- | --- | --- |
| `sketch.delete-geometry` | SKT-099/#518 | Delete selected authored curves with dependent relation repair |
| `sketch.solver-status` | SKT-100/#519 | Explain and recover from an overconstrained fixture |
| `sketch.intersection` | SKT-101/#520 | Retained intersection geometry from supported references |
| `inspection.interference` | DOC-037/#521 | User-facing solid collision and clearance inspection |
| `exchange.obj-output` | XCH-010/#522 | OBJ export with independent readback |
| `history.visibility` | REL-048/#523 | Active-preview visibility responsiveness, beyond accepted idle reuse |

`sketch.rectangle` and `sketch.circle` also now link the already open
SKT-094/#499 and SKT-095/#500 variant owners. Those were inventory mapping
omissions, not new work.

The following open implementation parents still combine multiple distinct
tools. Before pulling one, split its remaining accepted-v1 scope into
finishable Todo children, preserve its noncloud later scope, and keep the
parent as a cumulative integration gate or narrow it to a single result:

| Parent | Independent remaining tools to split |
| --- | --- |
| HIS-003 | Feature edit, suppression, rollback |
| SOL-008 | Body copy, geometry Align |
| SOL-009 | General body Mirror, rectangular pattern, circular pattern |
| SOL-013 | General Split, Draft, Scale |
| SRF-001 | Surface Extrude remainder, Surface Revolve |
| SRF-002 | Surface Sweep, Loft remainder |
| SRF-005 | Trim remainder, Extend, Untrim |
| GEO-001 | Construction-plane variants, datum axes/points |

Thirteen partial proposed rows currently point only to closed bounded slices:
`sketch.arc`, `sketch.slots`, `sketch.ellipse`, `sketch.conic`, `sketch.text`,
`sketch.fillet`, `sketch.mirror`, `solid.primitives`, `solid.combine`,
`body.rename`, `construction.midplane`, `construction.path-plane`, and
`construction.tangent-plane`. Their accepted subset is evidence, not proof
that every named variant is in v1. REL-030 must explicitly choose the minimum
v1 route for each; create a new child for any required remainder, and retain
deferred modes in the long-term roadmap. Common Boolean failure/retained-name
work in `solid.combine` deserves a narrow owner if included in v1.

Release validation remains distinct from tool implementation. In particular,
REL-012/013/014/015/016/018, GATE-005, CI-005/006/007/009 and the advanced
tutorial gates must still cover ordinary native input, failure recovery,
readable GPU presentation, representative-model responsiveness, exact
geometry/edit/undo/save/reopen/export, and actual packaged runtime on macOS,
Windows and Linux. The accepted local macOS Open/export receipts do not
qualify physical input or other platforms. STL and STEP are core to the
printable-part plan; 3MF export and modeled threads need explicit first-v1
disposition. No tag, build matrix or release promotion follows from this audit.
