# Active sketch history checkpoint

Verified 2026-10-12, isolated macOS hardware-accelerated tutorial candidate.
Accepted draft edits now have a session journal. Undo first cancels a pending
inline dimension editor, then an active tool, before traversing authored edits.
Redo requires Select with no pending input. Public behavior evidence is external
`knowledge/specs/sketch-draft-history.md`; journal architecture is an Alloy choice.

Hover, selection, rejected solves and unchanged drafts do not add entries.
A new accepted edit after Undo clears redo. Native annotation dragging coalesces
into one entry. Name edits participate; display units remain independent.
Finish remains one document transaction, and Cancel discards session changes.
`activeSession.draftHistory` reports index/count/canUndo/canRedo.

Evidence under `build-tutorial/evidence/`, outside git:

- `sketch-history-build.txt`: successful build.
- `sketch-history-ledger/report.json`: registered real-app workflow passes:
  pointer-created outline, constraint/dimension edits, cancellation precedence,
  rejected-change preservation, redo branching, Finish, Cancel and save/reopen.
- `history-native-receipt.json`: actual L and two clicks create a segment;
  Cmd+Z exits Line, next Cmd+Z removes the segment including its lazy fixed origin,
  Cmd+Shift+Z restores the exact authored IDs and relations. Initial empty sketch
  was opened through the API.
- `history-native-drag-receipt.json`: a dimension seeded through the API was
  physically dragged; one native Cmd+Z restores its exact original placement.
  The drag increases the journal by one entry, not one entry per mouse event.

Earlier attempts are not acceptance receipts: the first ledger attempt overlapped
an implementer's final test edit, and a subsequent successful workflow used
incorrect evidence-dimension field names. The final ledger run corrects both.
Concurrent instances of the same candidate bundle confused desktop-control
binding; native acceptance was repeated with API runners finished and a fresh
binding. Do not overlap native candidate control with same-bundle API runners.

Native text-editor-specific Undo behavior and every annotation drag interruption
are not claimed. No exhaustive edge-case sweep was required. The app's existing
geometry, API, native shortcut and storage checks provide bounded assurance.

Astra independently reviewed final source, 121-command ledger receipt, native
line/drag receipts and restored-profile capture. Bounded SKT-020 acceptance is
recommended with no material blocker; general shortcut parity remains separate.
