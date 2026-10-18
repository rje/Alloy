# Sketch Chamfer command record

Dated public research and creator/source URLs are outside the repository at
`../Alloy-reference-records/knowledge/specs/skt-096-sketch-chamfer-2026-10-18.md`.
The documented Sketch > Modify > Chamfer menu exposes Equal Distance, Two
Distance, and Distance And Angle modes. A corner vertex or two incident lines
define the selection. A preview and on-canvas handle/numeric entry adjust the
setback; Enter ends the tool. Resulting distance/angle dimensions are shown.

SKT-096/#501 owns one Equal Distance rectangle-corner workflow with an
editable equal setback, retained source changes, closed profile and printable
extrusion; this bounded subset passed [real-app validation](../../validation/sketch-equal-chamfer.md).
SKT-110/#534 owns two independently editable distances, and
SKT-111/#535 owns distance plus angle. SKT-112/#536 owns multiple equal-distance
corners in one sketch and passed [bounded validation](../../validation/sketch-multiple-chamfers.md);
SKT-113/#537 passed [bounded on-canvas drag and dimension-annotation
validation](../../validation/sketch-chamfer-canvas-edit.md). The
corresponding Chamfer Feature
Options switch must be implemented with those modes or explicitly split before
closing the children. Exact numeric defaults, per-stage Escape/repeat rules,
native line-order choice and legacy constraint remapping remain unknown from
current public evidence; report Alloy policy separately from observed behavior.
