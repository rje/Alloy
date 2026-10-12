# Decision 002: authored geometric references

Accepted for the first implementation foundation, 2026-10-11. This is a bounded
policy, not a completed topological naming subsystem.

Authored references carry producer feature UUID, body UUID, topology kind and a
generator role or operation-history lineage. Stable IDs belong to authored
objects. OCCT topology indices and transient handles are never serialized IDs.

For supported primitive generators, independently assigned semantic roles such
as `face:positive-x` and `edge:positive-x:positive-y` rebind to that generator's
new output after an upstream dimension edit. They do not mean "whichever face
is now farthest right" on arbitrary downstream Boolean/fillet results. The fixture
uses a 1e-6 mm role-comparison tolerance; OCCT's default 1e-7 mm linear precision is
kernel precision, not a global merge/repair allowance. Supported model dimensions
remain 0.001..100000 mm until later workflow evidence changes that range.

Operations retain OCCT generated/modified/deleted history and compose lineage
through the feature chain. Unchanged subshapes are absent from modified/generated
lists: first carry through the same-identity subshape if it still exists in the
new operation result and is not removed. Otherwise filter descendants to the
requested topology kind before considering uniqueness; generated edge history
can contain faces. One unique surviving descendant of the requested kind can
resolve a reference. No surviving subshape means unresolved/deleted; multiple descendants mean
ambiguous; merging distinct authored selections needs explicit repair. In those
states, dependent operations report a repairable evaluation error and must not
silently choose an index, nearest edge, or new boundary. The last successfully
evaluated state remains identifiable separately from stale intent. UI repair and
parameter-command failure behavior will be implemented with their real features.

The permitted OCCT-only fixture ran on pinned OCCT 7.9.3. Resizing a 10×20×30 mm
box to width 22 retained its generator corner/face roles. A radius-1 selected-edge
fillet produced generated faces and modified adjacent-face history on both boxes.
A notch split the selected edge into two descendants, a slab removed it, and
unifying two joined half-boxes mapped two source edges to the same merged edge.
All outputs passed BRepCheck_Analyzer. See
[fixture receipt](../validation/geometric-references.md).

Public foundations reviewed/retrieved 2026-10-11:

- [BRepTools_History pinned header](https://github.com/Open-Cascade-SAS/OCCT/blob/a016080bf6738d6aeae020badee4e888ad1540a5/src/BRepTools/BRepTools_History.hxx).
- [ShapeUpgrade_UnifySameDomain pinned header](https://github.com/Open-Cascade-SAS/OCCT/blob/a016080bf6738d6aeae020badee4e888ad1540a5/src/ShapeUpgrade/ShapeUpgrade_UnifySameDomain.hxx).

The fixture's conservative resolution policy is an Alloy design choice. Kernel
history does not itself solve persistent authored naming. DOC-012 implements
the initial resolver over evaluated body/history caches; MOD-003/MOD-004 and
their existing gates verify actual extrusion/fillet upstream edits and repair.
Cylinder seam roles, multi-body history composition, symmetry and reopen/recompute
reference persistence remain unresolved until those bounded tasks. No generic
exact-match or application workflow claim follows from this research.
