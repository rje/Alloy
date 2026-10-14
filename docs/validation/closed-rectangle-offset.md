# Retained closed rectangle Offset candidate

SKT-073 adds a bounded One Side Offset for four linked projected straight edges
that certify one complete axis-aligned rectangle. Each accepted group retains
the four source projection IDs, signed distance, and four derived corner and
edge IDs. The current projection context drives the corners on every solve;
the original purple projected edges remain locked and stop contributing a
second profile. An incomplete, rotated, repeated, or collapsed rectangle is
refused without changing the draft. Existing open-chain and circular Offset
routes remain available.

The adapted three-connector source has three independently retained groups at
outward 1 mm. Their overlapping and touching loops yield four disjoint
selectable cells with exact union area 973.5 mm². Selecting those four cells
for one Cut To Object removes material from the Case through its inner wall.
The shared planar union keeps exact contact-edge ancestry when the cells are
fused. The app run records the actual connector face ancestry, native
projected-edge click and typed distance, save/reopen, invalid edit refusal,
and a 0.25 mm retained plane edit with undo/redo. Evidence is in
`build-tut021/evidence/skt073/final-api` and `build-tut021/evidence/skt073/native`.

Changing just one group's clearance from 1 to 1.1 mm changes cell contacts.
The downstream Cut still names the old cell profiles, so finishing that edit
is refused atomically; this is an explicit current limitation. SKT-074 tracks
an opt-in aggregate selection with its own retained identity. The valid
retained plane edit demonstrates downstream propagation without changing cell
selection meaning. The API capture proves modeling; the focused Qt capture
proves the actual native Offset click, field focus and Cancel. Neither is a
claim about a physical keyboard or a proprietary reference runtime.
