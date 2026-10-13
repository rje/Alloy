# Bounded solid Shell contract

Evidence: public source `shell` in `docs/backlog/sources.json`, retrieved
2026-10-11; independent nested-cube source ledger and detailed evidence remain
outside the checkout in `../Alloy-reference-records/knowledge/tutorials/warwick/`.
OCCT's public `BRepOffsetAPI_MakeThickSolid::MakeThickSolidByJoin` documentation
specifies a solid, closing faces, signed offset and join type.

Documented workflow: open **Modify → Shell**, select a face to remove, use
**Sharp Offset**, **Inside**, and a positive **Inside Thickness**, then accept.
The selected face becomes an opening, the result is a retained solid, and the
operation appears in feature history. The source also documents body selection,
rounded offset, outside/both direction and multiple selected faces; these remain
outside this bounded implementation. Its default type/direction are unknown here.

Alloy's implemented input is one retained planar producer face, identified by
`{producerFeatureId, bodyId, faceId}`, on one active body. `solid.shell` accepts
`targetBodyId`, `removedFace`, `thicknessMm`, optional `featureId` for editing and
`name`. `solid.shellDialog` and `command.begin` use the same inputs. The dialog
shows Faces, Sharp Offset, Inside and Inside Thickness. Preview is disposable;
Cancel makes no authored or undo change. Invalid geometry refuses the commit.
The removed extrusion cap's stable ID identifies the resulting planar opening
rim, at the source cap frame, for downstream plane/sketch references. This is a
new named face, not the old top face shape. Other planar faces require the
separate face-naming work; no generic face parity is claimed.

Verification: derive a 25 × 25 mm XY rectangle, extrude 12.5 mm, remove its top
face with 2 mm inside thickness. One valid closed body has 3,182 mm³ volume.
Edit thickness and upstream extrusion depth, then check rim frame, undo/redo,
invalid thickness refusal, save/reopen, command preview/cancel and native view.
