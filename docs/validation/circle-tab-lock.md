# Circle diameter Tab lock candidate

SKT-041 / #226, candidate 2026-10-13. The permitted public tutorial is indexed
externally as `sweep-paper-clip-warwick-2017-20261013`; source URL, retrieval
date, and step ledger remain in
`../Alloy-reference-records/knowledge/tutorials/next-sweep-slice/paper-clip/`.
The observed source sequence is C, Front/XZ sketch, origin center, type diameter
2, Tab, Enter. The reference does not establish its internal constraint storage
or exact keyboard focus behavior.

In Alloy, Tab calls `sketch.circleDiameterLock` through the same command path as
the local API. A valid diameter stays in disposable session state with a circle
preview and visible locked-value cue. Enter places one circle with a driving
diameter; Escape clears the pending input without authoring. Ordinary typed
Return remains available. Leaving the tool or sketch resets the shared inline
editor so the next numeric entry is editable. Invalid values cannot lock or
author a circle.

The prior source-sequence probe after #222 failed before this fix: Tab left
`QApplication::focusWidget()` null, so the following key targeted no widget;
the Qt probe then failed when it attempted `QTest::keyClick` on that null
pointer. This was a real input-path gap, not evidence that Enter accepted.

`tests/circle_tab_controls.cpp` now passes the focused native Qt sequence
`sourceTabEnterAndCancel`: C, origin click, 2, Tab, Enter, followed by a second
diameter 3 Tab and Escape. The locked state has no draft edit, a visible GPU
preview, a read-only value field and readable Ø2 mm lock cue. Enter creates
exactly one radius-1 circle and a driving 2 mm diameter; Escape leaves that
geometry unchanged and restores editor editability. Exact authored save/reopen
passes. The persisted log is `build/evidence/circle-tab/native.log` (3 Qt stages
passed, 0 failed, 2383 ms); captures and saved model are in that directory.
The capture deliberately zooms the 2 mm circle eightfold so its native preview
can be inspected. This is injected Qt key/pointer evidence, not physical OS input.

`tests/circle_tab_api.py` independently passed through the Alloy executable's
local control endpoint. It used 23 shared command calls, including Preview,
Lock, Accept, another Preview/Lock/Cancel, and exact authored save/reopen. Its
`build/evidence/circle-tab-api/result.json` contains one XZ circle centered at
the origin with radius 1 mm and one driving 2 mm diameter. This API replay is
command and storage evidence, not a second native keyboard sequence.

Compatibility status: **matched** for the bounded source C/type/Tab/Enter
ingredient and Circle2 result under injected Qt events; **unknown** for physical
OS input, exact reference styling, and reference constraint storage. Full
paper-clip tutorial #225 remains a separate gate.
