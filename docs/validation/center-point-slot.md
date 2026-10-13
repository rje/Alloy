# Center Point Slot candidate (#251, 2026-10-13)

The public sketch lesson's straight slot starts at the origin, receives a rough
end-arc center and side-width click, then has its construction centerline made
horizontal, dimensioned 150 mm between arc centers, and one end radius
dimensioned R25. The independent source and command interpretation are recorded
outside the repository at
`../Alloy-reference-records/knowledge/tutorials/after-countersunk-plate/slot-command-contract.md`.
The public UI documentation establishes the center-first three-click order,
typed half-length, Tab to angle, width and Enter/Escape behavior. The numeric
initial defaults are unspecified there.

Alloy retains two semicircular arcs, two straight sides, a construction
centerline and an explicit center point. The origin snap adds Coincident;
Midpoint relates that center point to the construction line. Arc equality and
four tangent relations keep the boundary coherent. A `slotGroups` record
identifies the five retained entities and the center point, without preattaching
dimensions. The existing sketch solver and profile authority evaluate the
actual curves. The shared `sketch.centerPointSlot` command takes center,
end-arc-center and full-width inputs; the native Sketch > Create > Slot >
Center Point Slot path uses that same authority. Native typed half-length,
angle and width fields and pointer preview are bounded to this command.
During an endpoint drag the pointer cue is a rubber line; the solved capsule
updates on release. The drag is disposable until release and Escape, tool,
history or session changes discard it.
After independent review found an Escape-before-release path that could still
commit the pointer move, a focused native Qt case passed in 1.181 s: press the
retained guide endpoint, move, press Escape, release, and compare the complete
authored draft before/after. The tool/history/session/draft teardown paths now
clear the same pending drag token. The original full native sequence and API
run were not repeated for this localized teardown change.

The focused core contract passes rough slot, origin snap, actual free end-center
move before Horizontal/Vertical, then DOF 3 → 2 → 1 → 0 after Horizontal,
D150 and R25. It asserts the 200 × 50 mm closed profile and area
`7500 + 625π` mm², retained IDs, a later centerline-length edit to 170,
undo/redo, save/reopen, and atomic oversized-radius refusal. The native Qt
fixture passes the real menu/action, three pointer clicks, typed half-length →
Tab angle and width, actual guide endpoint pointer drag, Horizontal/Vertical
guide selection, D150 and R25 creation, Finish, save and reopen. It begins with
one harness activation and asserts the resulting focus before later typing;
it does not repair focus or selection between demonstrated steps. The settled
typed and fully constrained captures show legible palette text, analytic
outline, construction guide and dimensions. Native evidence is in
`build-slot/evidence/native-pass/`; the focused native test passed in 8.23 s.

The separate real-app shared-API run passed in
`../Alloy-reference-records/deliverables/center-point-slot-2026-10-13/api-attempt-01`.
It checks rough creation, Horizontal, D150/R25, profile, D170 edit, exact
history and reopen. Its executable SHA-256 is
`51f99a37160adbfd89fccd7fcba60891ca3b333feadaa0aeec4e4598788fa9a4`.
The API run does not stand in for native input, and this candidate does not
claim the complete multi-page lesson, arc slots, or arbitrary slot variants.
The native test's earlier saved crash was a temporary `QJsonValueRef` lifetime
error in its save/reopen assertion, fixed by owning the JSON value; the model
had already saved and reopened. A transient palette overlap in an immediate
capture cleared after queued layout events; the passing fixture asserts
non-overlap before its settled capture.
