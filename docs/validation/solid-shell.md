# Bounded inward Shell acceptance

2026-10-12, SOL-024 #185. Sol implemented; Astra independently verified the
integrated candidate. One selected named planar opening, sharp inward thickness,
shared feature/history/API and readable dock are accepted. Broader Shell modes
remain #51.

The 25×25×12.5 mm source with 2 mm walls evaluates to one valid closed material
solid of 3182 mm³. The source-specific rim reference follows height edits.
Focused document checks cover thickness/source edits, invalid thickness/missing
face atomic refusal, undo/redo and exact storage.

Astra's real-app run separately verified actual geometry ray-picking, preview,
Cancel preservation, commit, 1.5 mm thickness edit (2488.5 mm³), undo/redo and
2 mm save/reopen. Captures show readable controls and a real hollow solid on the
Apple M4 hardware viewport. Final fixes require source-face history or every
exact source boundary edge for rim identity, display actual 0/1 selections,
preserve empty selection state, and route capture.dialog to the Shell dock.

The verifier-authored `shellFaceThicknessCancelCommit` Qt widget test passed:
actual Shell toolbar and viewport mouse events, spinbox keyboard entry/Return,
preview, Cancel and OK. This proves Qt event routing, not OS-level pointer input.
Two CUA native-pipe startup failures leave OS input explicitly unverified; no
successful native-computer-control run is claimed.

Evidence: `build-tutorial-polish/evidence/shell-independent/` contains review.json,
final-smoke.json, qt-input.txt, saved model and actual captures. The final smoke
checked only the corrected labels/capture/rim, avoiding an unchanged full rerun.
The complete nested-cubes tutorial remains open.
