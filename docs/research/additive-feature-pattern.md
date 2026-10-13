# Additive feature Rectangular Pattern

SOL-027 / #191 follows public reference tutorial-toy-block-2026, steps 07
(12:30–14:25) and 13 (17:58–18:23). Source records, creator, URLs, dated retrieval
and original-video frames are external in the toy-block tutorial ledger.
Observed: timeline **Features**, one/two straight edge axes, **Extent**, **One
Direction**; stud quantity 4 over 24 mm and 2 over 8 mm; tube quantity 3 over
16 mm, **Identical**. Quantity includes the seed. First-pattern Compute Type is
not visible; Identical is this bounded implementation's documented choice.

`solid.rectangularPattern` / `solid.rectangularPatternDialog` accept
`sourceFeatureId`, `targetBodyId`, ordered
`axes: [{edgeId, quantity, extentMm}]`, optional `name`, optional editing
`featureId`. Signed canonical-mm extent follows each retained edge's actual
start-to-end direction. The slice accepts one-target, one-profile Extrude Join
seeds and one/two perpendicular straight target-body edges, 2–100 instances per
axis and at most 1000 total. It supports Extent/One Direction/Identical without
suppression. Other operations and circular patterns remain in #54.

The seed contribution is a real OCCT profile prism, not the whole result body.
Copies translate that contribution by `i*extent/(quantity-1)` and fuse into the
same active target. No replacement body or source-profile guessing occurs.
Unavailable axes, unsupported seed references and disconnected results fail
atomically. Only unique kernel edge descendants remain available for downstream
references; unresolved or split edges are never guessed. Repeated previews and commit reserve the same feature
ID; editing, upstream seed changes and save/reopen keep authored references.

Native Create → Rectangular Pattern opens the shared panel. With the command
active, timeline clicks choose the source feature (`selection.featureSet`).
Select Axis buttons route viewport straight-edge picks through the same axis
selection (`selection.patternAxisSet`, zero-based axis). Quantity, Distance,
Flip and Remove Axis 2 alter the same session used by `command.update`. Valid
previews show real repeated geometry and GPU axis arrows; OK/Cancel/Escape
commit one feature or restore the exact authored document. This slice uses
numeric/Flip extent entry; direct arrow dragging is not implemented.

Instances retain UUIDv5 identities from pattern feature and integer indices.
Each copied solid edge and support boundary carries optional `occurrenceId`
while keeping the true source sketch/entity IDs. Cap and boundary IDs are per
occurrence; original seed lineage stays unchanged. The projection/Shell roof
work in #196 consumes this explicit occurrence identity separately.

The focused `feature_pattern` test builds eight studs, checks exact prism
material and eight current base/end rims, seven copy occurrence IDs, six-stud
count/extent editing, seed editing and exact history/storage. It continues
through 1.49 mm Shell, three annular tubes and the source-defined nineteen-edge
0.2 mm Fillet with one valid closed body. The tube plane is numerically authored
at the correct roof height; this core fixture does not claim native roof-center
or retained sketch Offset behavior. `tests/feature_pattern_api.py` covers the
shared actual-entrypoint command/panel chain; full native/tutorial/visual
qualification remains separate.
