# Existing-circle pair Symmetry — SKT-121

Accepted 2026-10-18 for two separately authored circles about one straight construction axis through the shared Symmetry command and solver. The retained relation binds both center IDs and equal radii. It is independent of Sketch Mirror. Existing line and arc Symmetry cases continue to pass.

The real Alloy fixture fixed the axis, dimensioned horizontal center separation to 40 mm and retained exactly one free translation DOF. Diameter changed 20→24 mm while both centers moved together along the axis direction and stayed reflected. A conflicting diameter edit refused atomically. Undo/Redo and saved authored reopen matched. Focused solver tests cover initially unequal radii, source edits, retained center correspondence and refusal of tampered correspondence.

Native Qt input passed the line, arc and circle rows, including circle/circle/axis picks and a visible selectable/deletable badge, without inter-step focus repair. Inspected app captures show the symmetric circles and readable diameter on an Apple M4 hardware-accelerated OCCT OpenGL 4.1 Metal viewport. Physical OS input, exact reference glyph styling and other-platform runtime remain open.

Evidence and saved model: `../Alloy-reference-records/validation/circle-symmetry-2026-10-18/api-one-dof/` and sibling `native/`. Dated public training evidence and remaining unknown group-selection behavior: `../Alloy-reference-records/knowledge/specs/skt-022-remaining-curve-pairs-2026-10-18.md`. One-shot multi-pair Symmetry remains later-roadmap SKT-122.
