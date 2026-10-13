# Face Sphere independent review

Reviewed 2026-10-13 on candidate db3e0ee. Functional controls/API accepted for
bounded SOL-014/#145; visual acceptance remains pending the red-cutter correction
below. The reviewer did not implement the feature. Full die/component integration
and Left/Back Browser routes require the later integrated gate.

The actual-app `sphere_controls_api.py` run passed exact face and center ray
routing, default Cut, preview/cancel, commit, retained body identity, undo/redo,
edit-preview cancellation, accepted diameter edit, Repeat center reset and last
accepted diameter, invalid off-face refusal, stable New Body preview identity,
and exact save/reopen. The private Qt runtime candidate exited normally.

The focused `sphere_controls_test` was extended independently to use Repeat on
a different face. It passes (3/3 including setup/cleanup, 2.033 seconds): actual
viewport face/center clicks produce default Cut; actual Cancel preserves stock;
typed diameter 4 is accepted; OK creates the first hemisphere. A Qt context-menu
event on the +X face, followed by a real menu-action click, opens Repeat Sphere
with that different support, cleared u/v, retained diameter 4 and default Cut.
Actual center click and OK create the second hemisphere, with total removal
32π/3 mm³ from the 8000 mm³ cube. This is Qt-dispatched input, not physical OS
input. The ordinary native context menu is an explicit adaptation of the public
radial Repeat menu. Parameter carry-over is an Alloy choice, unknown in source.

Enter interpreted the numeric value but did not activate the commit shortcut in
this inactive macOS test window. The test explicitly records this and uses the
actual OK button; Enter-to-commit is unverified, not a claimed matching gesture.
The source permits Enter or OK. No OS-level keyboard claim is made.

The controls are readable at 1180 × 760 and show one selected face, local center,
diameter, Cut/New Body, Cancel and OK. However the cutter appears purple/magenta
when transparent red is composited over the blue-selected stock. The source
explicitly describes a red cutting preview. Required bounded correction: present
neutral stock during valid Sphere preview (with a distinct face cue as needed)
so the cutter reads red; inspect one corrected actual capture before closing
visual acceptance. No matched final-model screenshot exists in the source.

Evidence: `build-sphere/evidence/sphere-independent/` contains `report.json`,
`transcript.json`, `sphere-pips.alloy`, `sphere-red-cut-preview.png`,
`sphere-pips-committed.png`, `qt-input.txt`, and
`qt-different-face-preview.png`. The 21-pip kernel gate was already verified
separately and was not repeated in this controls review.
