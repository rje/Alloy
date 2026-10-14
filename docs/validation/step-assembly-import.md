# Retained solid STEP assembly import candidate

XCH-009 extends the existing File > Open From Computer and `document.importStep`
entrypoints to one XDE assembly root containing named, closed millimeter solid
leaves. The embedded STEP bytes and one source-derived definition/occurrence
graph are authored in a single `stepAssemblyImport` history feature. Repeated
references share a definition ID but retain separate full-path occurrence and
body IDs. A rebuild checks the embedded hash, reparses the source, and compares
the complete graph before evaluating separate world-space leaf shapes. Each
component placement is applied once; the same leaf shapes feed rendering,
picking, geometry inspection and per-body export. Imported hierarchy is
read-only and separate from editable local component definitions. The Browser
and `component.inspect` use the authored graph. Imported planar face and edge
references include occurrence identity; their indexed topology is scoped to
the immutable embedded source and verified snapshot.

The focused core test constructs a nested, rotated and translated group with
two occurrences of one block definition. It checks exact world bounds,
shared-definition/distinct-occurrence IDs, occurrence-qualified references,
one history entry, save/reopen without the source file, undo/redo and atomic
snapshot rejection. A named one-leaf assembly also stays on the XDE path.
Only proper rigid component placements are accepted. An external qualified
board fixture checks the complete root, five named groups, twelve solid
leaves and one named connector's world bounds. The existing single-solid STEP
test still covers inch conversion and single-body behavior. A plain
unstructured two-solid STEP file is rejected without inventing hierarchy.

The 30-call real-app API run imported that board fixture, inspected the one
authored graph through `component.inspect`, selected all twelve leaf IDs,
exported a connector leaf and independently read its world bounds from STEP,
then exercised undo/redo and exact saved reopen. A focused Qt native test
physically clicked an imported Browser leaf under a local component and
checked the selection, nested hierarchy and undo/redo. GPU captures show the
separate board and connector solids with readable group/leaf names. The
independent bounded compatibility review accepted this candidate. Viewport
ray-picking of a moved imported occurrence remains source-reviewed only and
belongs in the later insertion/reference gate. Evidence is external at
`../Alloy-reference-records/deliverables/step-assembly-import-2026-10-13/`.

This child does not add imported component editing, joint insertion, colors or
materials, whole-assembly STEP re-export, or general interchange fidelity.
Those remain in the tracked assembly/interchange parents. Public provenance
and the fixture qualification remain outside the repository in the Alloy
reference records; no reference application was used.
