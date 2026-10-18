# Ordinary bracket input: Extrude focus continuation

2026-10-18, local macOS arm64 Release `build-preview-release`, Qt 6.11.2.
This is the accepted REL-051/#586 prefix of REL-012. Independent Sol review
replayed the unchanged candidate (Qt 3/3), matched saved model and STL hashes,
independently parsed the mesh and inspected the controls/captures; no blocker.
The complete
sketch-to-export native workflow remains open at export modal activation.

The existing saved fullsize illustrated bracket now opens after GEO-006's
circular-seam provenance repair. Through actual Qt-delivered input, one private
application completes timeline sketch activation; naturally focused C, center
pick, typed diameter, Tab lock, Escape cancellation and viewport return; Finish
without authored change; timeline Extrude edit, typed 51 mm/Tab preview/Cancel;
reopen, typed 51 mm/Return commit; immediate native Undo; Browser body selection;
and active context-popup Escape dismissal. Initial activation and API camera
setup are distinct from these user steps. Pointer tests use the widget actually
hit at the target pixel. Keys go to the observed natural focus widget, with no
interstep harness focus or selection repair.

Before the fix, Return committed the new depth but left keyboard focus outside
the viewport (`resume06`). `finishSession` now hands keyboard focus to the canvas
when closing Extrude, for commit and cancel, consistent with the existing other
modeling commands. The candidate verifies both handoffs and delivers the next
Undo without repair. Undo restores the exact source authored document.

`tests/ordinary_bracket_input_controls.cpp` reproduces this prefix with
`ALLOY_ORDINARY_INPUT_MODEL` pointing to the external saved bracket and
`ALLOY_ORDINARY_INPUT_EVIDENCE` pointing to a new external directory. The test's
subsequent shared-API STL export and independent stdlib mesh readback are separate
from native export input. They do not qualify a native file chooser. The readback
finds 3,640 triangles with paired oriented edges, finite aligned normals and
positive volume 4,694,744.870 mm³ against analytic 4,694,718.448 mm³ (about
0.00056% difference). Maximum mesh/analytic bounds difference is 0.002923 mm,
within the requested 0.05 mm surface deviation. This is tessellation evidence,
not exact mesh bounds equality.

The preserved full-route mode (`ALLOY_ORDINARY_INPUT_REQUIRE_MODAL=1`) continues
to Browser Save As Mesh. Its popup closes and the sheet becomes the active modal,
but after a bounded readiness wait keyboard focus remains `QTreeWidget
browser.tree`, outside the dialog. This is the first unresolved boundary. Both
immediate and queued activation attempts failed and were discarded. Do not
repair focus in the test. A Todo continuation must qualify natural modal field
focus/input, Tab/Escape/Return, cancellation and return, then actual native OS
Save/Cancel chooser and independent output readback on the same fixture.

External evidence root:
`../Alloy-reference-records/validation/rel012-ordinary-input-2026-10-18/`.
`resume06` preserves the pre-fix Extrude failure; `focus-fix02` records the
export modal keyboard recipient; `modal-queued01` preserves the final rejected
activation attempt; `accepted01` records the passing bounded candidate, screenshots,
saved model and STL. `review01` records the independent rerun. The full failing driver is preserved as
`full-gate-modal-repro.cpp`. Earlier fixture-open failures and harness corrections
remain separate in the same evidence root. No physical human input, packaged
qualification, other-platform runtime or exact reference visual parity is claimed.
