# Binary STL normals at serialized precision

2026-10-13, PRT-009/#269. Cumulative-05 original binary STL passes strict readback
with36,952 facets. Edited offset−4 has33,410 facets and one normal mismatch:
facet6131 near z79.76, cosine0.99997325756 (0.419 degrees). Its twice-area is
8.326e-5 mm2 and its longest edge is0.1484 mm. Winding is not inverted.

A planar numerical witness matching the stored normal changes only vertex z by
+2.822e-6/−2.822e-6/−0.425e-6 mm. All changes are inside half of the float32 ULP
(3.815e-6 mm), and every vertex rounds to the identical serialized bytes. This
establishes compatibility with coordinate rounding, not a recovered original
mesh. Independent review verifies the witness and recomputed normal alignment.

The existing staged binary postpass now derives each retained unit normal from
its actual serialized float vertices. Only the12 normal bytes change; all38
coordinate/attribute bytes, order, winding and counts remain intact. Existing
exact-collapse checks, detached tessellation, units, settings, atomic publication,
ASCII handling and authored geometry remain unchanged. No checker tolerance,
area threshold, vertex movement or CAD healing is introduced. Earlier receipts'
retained-normal byte identity is historical evidence, not the current policy.

A quantization-sensitive closed triangular prism exercises actual MeshExport in
binary millimeters and inches. Before the fix, it fails the existing strict
normal/winding checker. Afterward its eight facets form one closed positive-volume
component; exact cap vertex bytes remain, no facet is removed, and the original
BRep is byte-identical. The new regression, existing pole and multibody export
checks pass3/3 in2.55s; the application build passes.

External evidence under `validation/ornament-feasibility/`:
`stl-normal-diagnosis.json`, `stl-serialized-normal-before.log`,
`stl-serialized-normal-final.log`. Actual saved original/edited export validation
is performed separately by root's targeted runner; the accepted creation movie,
history and reopen evidence are reused. No editor was opened for this correction.
