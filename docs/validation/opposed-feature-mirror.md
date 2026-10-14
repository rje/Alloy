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
cantilever. Its source sketch requires a cutout side-face support that the
current Cut does not publish: the actual cut evaluation contains no retained
support face for its Y17 or Y23 side wall. The source workflow's side-face
offset, Join, finishes, downstream Mirror, and physical fit remain pending.
The XZ five-feature behavior is unchanged by this extension.
