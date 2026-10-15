# REL-019: synchronous geometry feedback

Alloy paints a status-bar message and installs a wait cursor before selected
synchronous geometry commands: “Computing … Input resumes when finished.
Cancellation unavailable.” Completion or refusal restores the prior cursor and
shows elapsed wall time. Nested authoritative commands share one indication.
The guard covers Box, Extrude, Combine and Fillet, their initial modeless
previews, session preview/update/commit, Fillet picking, feature edit/reorder/delete,
sketch Finish, history undo/redo and document Open. Native Fillet input callbacks
now request the existing shared preview command rather than evaluating after
the guarded update has returned.

This is feedback, **not background evaluation**. Kernel work still blocks the
GUI thread, animation and local API request handling. There is no event pump,
percentage, time estimate or inactive Cancel control. The status is repainted
before entering evaluation; OS compositor delivery during a prolonged kernel
call is not independently established by the short fixture. General
responsiveness and interruption remain REL-016 work.

`app.state.geometryWork` exposes `active`, `cancellable: false`, and `last`
(`command`, `label`, `elapsedMs`, `succeeded`). `active` is available to synchronous
observers; a separate API client cannot poll it while the GUI thread is blocked.
Clients can read the last completed operation afterward. Timing includes command
execution and presentation refresh, and is not a kernel-only benchmark.

Focused CTest passes 1/1 (Qt reports 3/3 including setup/cleanup), 2.86 seconds,
in `deliverables/geometry-busy-feedback-2026-10-14/qt01/` in the external records.
The exact `ctest.log`, `computing-status.png`, `completed-box.png` and
`busy-box.alloy` preserve the evidence. The normal app was relinked in the same
build. The fixture uses real API commands on a small
40 × 30 × 10 mm box, a refused feature edit and a valid R1 initial Fillet
preview/Cancel. It checks restored cursor-stack ownership, cleared busy state,
completion/refusal metadata and unchanged authored/history state on refusal.
Its busy image is an actual rendered Qt status-bar capture during the synchronous
message notification; a separate normal capture verifies the completed GPU
model. No artificial delay or repeated long enclosure evaluation is needed.
The status strip was inspected and its whole message is readable. The completed
GPU box is readable; the existing unused right canvas band remains REL-013 work.
Initial preview, success and refusal are exercised; exception cleanup and nested
ownership are source-reviewed. This gate does not establish physical pointer
input or a successful repaint during a multi-second blocked kernel call.
An independent reviewer checked source, snapshot hashes, focused assertions and
both captures; no further runtime was requested.
