# Opposed Feature Mirror plane

Origin YZ is available alongside the existing origin XZ option in the shared
Feature Mirror command and native selector. Cut tools and the retained
five-feature Join/Chamfer/Fillet group use the same selected reflection plane;
existing origin XZ documents keep their authored plane.

The focused Cut core test passes with a YZ preview and the existing XZ edit,
history, and reopen path. A real-app API run on the accepted split enclosure
created a 6 × 3 mm catch through its 3.5 mm wall and mirrored that Cut to the
opposed long wall across YZ. Each operation removed 63 mm³. A focused Qt test
selected Origin YZ through the native combo, checked preview and Cancel, then
committed the Cut Mirror. Captures and command ledger are outside the repository
under `../Alloy-reference-records/deliverables/enclosure-snap-2026-10-13/` in
`opposed-cut-01` and `sol053-native-01`.

The five-feature YZ snap group still needs proof on the actual retained
cantilever. GEO-013 now supplies the required exact cutout side-face support
and retained offset plane. SOL-054 supplies the next prerequisite: replaying
the connected selected-region Join created by the projected sketch boundaries.
The actual finished snap, downstream Mirror, and physical fit remain pending.
The XZ five-feature behavior is unchanged by this extension.

The bounded opposed-Cut source was independently reviewed and integrated at
`62d164a`; the main application build and focused Cut regression pass.
SOL-053 remains open for actual five-feature YZ snap qualification.
