# REL-015: packaged actionable-failure slice

Accepted 2026-10-18 for saved-fixture, local-control, file/history and
packaged macOS GPU behavior. Native physical-input discoverability remains
REL-050/#539; this receipt makes no physical input claim.

The cumulative runner
[`tests/actionable_failures_api.py`](../../tests/actionable_failures_api.py)
used a staged macOS arm64 package, binary SHA-256
`fabd9b21d5871ca8d5201f50635f5770f99908e8188ce8be14ea77d855de67da`
and archive SHA-256
`1bd49ed037fc130ab81a1d473ac21cb0618293c549961308021f2d213ff8b8b1`.
It passed 101 real-app calls across five small fixtures:

- An over-thick named Wall edit on a 40×30×20 mm Box/open Shell now refuses
  with `parameter_evaluation_failed` and an instruction to reduce thickness
  or edit the source. OCCT had previously returned unchanged stock as a
  valid Shell. A source guard now requires inward open Shell to remove
  material; the focused core regression passed. Updating Wall to 3 mm then
  passed, with exact authored Undo/Redo.
- Deleting Shell from a face-supported inner-floor sketch refuses because the
  support producer is missing. Deleting it from a front sketch with a linked
  inner-edge Project refuses because the projected edge is no longer a real
  boundary. Both failures leave the saved source and authored state intact.
  A valid Shell thickness edit succeeds and Undo/Redo/reopen pass. This is
  preservation and continued editing of the existing source, not repair of
  a committed broken document or source reassignment.
- Committing a prepared Box edit after the document units change refuses with
  `stale_session` and says to cancel and restart. Cancel/reopen of the editor
  and Undo of the unit change pass through the local control API.
- A retained Section whose face becomes ambiguous after Split reports
  unavailable rather than clipping an arbitrary descendant. Undo Split
  restores availability; Redo and reopen reproduce the unavailable state.

Each refusal compares authored data, evaluated summaries, revision and undo
state to the last good checkpoint and confirms the original saved file's
SHA-256 is unchanged. Reopen compares authored data and evaluated summaries;
exact BRep equality was not independently checked. The actual GPU capture
shows “Side (unavailable)” with unclipped geometry. The packaged app reports
Apple M4, hardware acceleration, OCCT OpenGL 4.1 Metal. Full transcript,
fixture hashes, model files, screenshots, package manifest and regression
observation are at
`../Alloy-reference-records/validation/rel015-actionable-2026-10-18/`.

Astra independently reviewed the source change, transcript, fixture hashes,
package binary hash and captures. It found the bounded API/package claim
supported and no material source blocker. The stale-dialog capture shows no
visible error because the tested action used the API. Computer control could
not attach to the staged window (`cgWindowNotFound`), so the physical native
error/recovery sequence is Todo REL-050/#539. Clean-host and Windows/Linux
qualification retain their separate release owners.
