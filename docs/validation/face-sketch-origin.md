# Fresh planar-face sketch origin candidate

SKT-075 adds `originConvention: "projectedOwnerV1"` only to freshly created direct
planar-face sketches. The origin of an admitted identity/rest owner component is
projected orthogonally onto the exact retained support plane. The existing face
axes, support IDs, local point coordinates, solver origin constraints, and
origin-snap authority are shared by creation, history evaluation and linked
Project. Sketches without the field retain their original face-frame origin;
they are neither migrated nor rebased during edit or reopen. Construction-plane
sketches retain their current convention.

The modeled source and support must have unambiguous identity occurrence
contexts at creation. New sketches on placed or duplicated component contexts
refuse before commit. Later copies do not invalidate an existing sketch. This
bounded slice does not author directly on a transformed or posed instance.
Face-axis polarity and the arbitrary construction-plane origin remain open
compatibility questions.

`face_sketch_origin` core checks an offset face, actual projected point and
origin snap, linked Project coordinates, upstream support move, save/reopen,
legacy coordinates, later copy and atomic refusal of a newly ambiguous sketch.
`face_support` continues to pass for the legacy convention. The real application
API run in `../Alloy-reference-records/deliverables/face-sketch-origin-2026-10-14/api-03`
checks fresh creation, a snapped center rectangle and save/reopen. Its screenshot
shows the origin and rectangle on the displaced face. Native pointer snapping
is not qualified by these checks and remains a separate input gate.

Independent Astra review accepted `7c29a61`, integrated at `62b97d7`. Main application build and both focused core checks pass. Native pointer-origin snapping is separately tracked in SKT-077/#375; complete enclosure replay after script updates remains unperformed.
