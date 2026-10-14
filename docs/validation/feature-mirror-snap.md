# Retained snap-group feature Mirror candidate

SOL-051/#327 mirrors the selected source group in authored history order:
one Extrude Join, three simple Chamfers, and one simple Fillet on one target
body across origin XZ. The authored Mirror stores only the five source UUIDs,
target UUID, and plane reference. Evaluated Join prism, cap, edges, and the
pre-finish edge shapes are disposable source-stage caches. Reflection reuses
the existing Boolean Join, Chamfer, and Fillet evaluators. Each finish selects
the unique current straight edge with the reflected source endpoints and full
length. It refuses missing or ambiguous descendants and intervening
modifications to the target; independent Bottom work may occur between the
source group and Mirror. This is a bounded identical copy of the one-prism
Join and its four finishes, not a general feature replay or Adjust mode.

The source-shaped headless proof uses a rounded, shelled, split case with a
stepped snap sketch. It verifies a valid closed Top after all five mirrored
operations, reflected cap and edge identities, cap frame on the opposite
side of XZ, independently expected added volume, source Join edit, undo/redo,
save/reopen, same-target gap refusal, and a valid unrelated Bottom Fillet
interlude. Prior Cut Mirror, Chamfer, and Fillet focused core checks pass.

Native Timeline selection, real-app API operation, captures and independent
review are pending. The external source fixture is at
`../Alloy-reference-records/deliverables/join-edge-lineage-2026-10-13/core-01/source-snap.alloy`.
The broader SOL-049 feature Mirror family remains open.
