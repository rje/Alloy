# Independent document tabs checkpoint

DOC-015 (#151), 2026-10-12. Candidate: build-tutorial/alloy.app, private identity.

## Implemented

Plus and Cmd/Ctrl+N create independent local designs. Stable tab IDs route activation
and targeted closure. Each design owns its document/history, accepted sketch draft
and local undo journal, camera and selected IDs. Switching resets shared viewport
and widget caches even for equal revisions. Pending commands/input refuse switching
or active closure. Save requires Finish/Cancel for drafts; quit checks every tab.
Last-tab close creates a fresh blank design. Native file open retains its explicit
replace-current policy. No reorder/detach support is claimed.

## Evidence

- Incremental build passed.
- `build-tutorial/evidence/document-tabs-ledger/report.json` passed the coherent
  real-app workflow: same-revision independent bodies with exact rendered IDs and
  evaluated bounds, camera/selection/history retention, accepted sketch draft
  journal restoration, pending sketch/solid guards, inactive dirty quit refusal,
  dirty close, and both saved designs reopening.
- First attempt stopped at a test-only assumption that an empty dimensions array
  was always serialized. The final test accepts absence as empty after undo.
- Astra identified and verified the fix for active-command closure retaining
  stale command controls after destroying a document. No other source blocker.
- Native plus and A/B/A tab clicks restored the expected browser/body. API-seeded
  box/cylinder are setup, not a native modeling claim. Native inactive dirty close
  Cancel exposed an active-tab restoration issue. The corrected final run verified
  Cancel retains A and both documents, Discard removes only B while keeping A, and
  Cmd+N adds a blank independent design. See `tabs-native-final-receipt.json`;
  native Save-close is not separately exercised (shared Save has API evidence).
- `tabs-native-a.png` is a real API capture; initial background CUA screenshot was
  stale until raising the candidate. Do not use that stale image as visual evidence.

## Limits

An unchanged open sketch is conservatively dirty. Exact visual parity and every
unfinished-input behavior are not claimed. Cancel/Discard and Cmd+N are qualified above; native Save-close remains separate.
No full tutorial completion, release or CI run.
