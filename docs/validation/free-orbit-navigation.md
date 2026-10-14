# Free Orbit navigation (NAV-001)

The public navigation contract is recorded outside the repository at
`../Alloy-reference-records/knowledge/specs/viewport-navigation-compatibility-2026-10-14.md`.
It documents Shift + middle-button orbit and distinct Free/Constrained modes;
the initial mode default and pointer sensitivity depend on settings or are
unspecified. Alloy now offers an explicitly labeled **Free Orbit** control and
Shift + middle-button drag. Its initial pivot is the fitted scene center. This
is an Alloy policy, not a claim about the reference's selected/automatic pivot.

The shared `view.orbit` command rotates the real OCCT camera and retains its
direction/up as a custom view through redraw and tab switching. The same camera
state is reapplied by the resize path; resize-specific native input was not
repeated in this gate.
The visible checked Free Orbit control lets a user without a middle button orbit
with a left canvas drag; Escape exits. Navigation gestures run before modeling
press handlers. Home and a named orientation restore a fixed camera. Existing
middle-button and Option/Alt-left pan remain separate.

The focused production-theme Qt test `navigation_free_orbit_controls` passed
3/3 on macOS 26.5.1, Qt 6.11.2, OCCT 7.9.3 and Apple M4 OpenGL. An asymmetric
40 × 20 × 12 mm solid made camera changes visible. Delivered Qt pointer input
exercised middle-button pan, Shift + middle-button orbit, and the actual checked
Free Orbit button followed by left drag and focused Escape. It asserted camera
direction changes only for orbit, scene-center retention, custom-pose restoration
after a tab switch, Home reset, and unchanged selected body, authored design,
document revision and undo journal. The actual GPU captures show a readable
selected Free Orbit control and changed solid view.

Exact log, both GPU captures and editable model are under
`../Alloy-reference-records/deliverables/navigation-free-orbit-2026-10-14/native-01/`.
An initial test run timed out when the dirty test window opened its Save dialog
at teardown; explicit fixture save resolved that harness issue. The accepted
run completed in 3.21 seconds. This is Qt-delivered input, not OS-device,
trackpad, frame pacing, or cross-platform acceptance. Constrained orbit,
selected/automatic pivot, trackpad gestures, ViewCube and projection preferences
remain open navigation work.
