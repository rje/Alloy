# Single-solid STEP checkpoint

DOC-011 implements an embedded-source `stepImport` history feature and explicit
single-body STEP export. File > Open From Computer accepts `.step`, `.stp`, and
`.ste` into a new design tab; the shared `document.importStep` command can also
append to the active design. File > Export opens a body and unit picker, then
uses the same `document.exportStep` command as local automation. Export supports
millimeters and inches and does not mutate authored history. Imported source
editing is refused clearly; later features remain editable. This slice covers
one valid closed solid, not assemblies, product structure, metadata, or XDE.

The isolated `exchange-finish` candidate builds with OCCT 7.9.3 and private Qt
6.11.2. `ctest -R '^step_exchange$'` passes. `step_fixture` directly uses OCCT
STEPControl, without linking Alloy's interchange code: it authors a 25.4 ×
50.8 × 12.7 mm inch-file box, authors a separate two-solid rejection fixture,
and independently reads exported mm and inch files in another process.
`tests/step_exchange_api.py` exercises the real app API, including malformed and
two-solid atomic refusal, one undo/redo entry, SHA-256 embedded source policy,
independent export readback with valid bounds/volume, and native document reopen
after deleting the original STEP file. The DOC-011 workflow ledger receipt is
`build-step/evidence/step-ledger-reviewed/report.json` in that isolated worktree.

In a separate private native candidate, File > Export opened a readable modal
sheet showing the selected imported body and explicit millimeter unit choice.
Tab to Save and Return opened the native file chooser; the saved STEP file was
read by `step_fixture` in another process. That readback reports one valid
solid, millimeter units, bounds [0, 0, 0] to [25.4, 50.8, 12.7] mm within
0.0000001 mm tolerance, and volume 16387.064 mm³. The private evidence files
are `build-step/evidence/step-native-receipt.json` and
`step-native-reader.json`. Physical change of the unit dropdown and native
File Open of a STEP source remain unqualified; both units and import are covered
through the shared real-app API. This is not broader CAD interoperability or
assembly/product-structure support.

