# Local committed-work recovery

REL-014 adds a private, atomic snapshot of each dirty tab's **committed authored
document**. Each running editor owns an exclusive session lock. After an
interruption, File → Recover Work… lists stale snapshots and opens a chosen
valid record in a new, unsaved tab. The original path is informational and is
never overwritten by recovery. The new tab receives its own durable snapshot
before the stale one is retired. Save As keeps the recovered work; successful
save or explicit discard cleans the owned snapshot. Active uncommitted drafts
are outside this bounded recovery contract.

The same authority is available through `document.recoveryList`,
`document.recover`, and `document.recoveryDiscard`. Snapshot schema, hash,
private-file and live-session checks refuse malformed, newer, or live work
without changing the open document. A failed snapshot write is reported in
command results, `app.state`, and a persistent status-bar warning. A failed
or cancelled save leaves a recovered tab dirty, even though its initial history
is clean.

Focused evidence (2026-10-14):

- `recovery_store` CTest passed 1/1: atomic per-tab writes, live exclusion,
  durable transfer ordering, and malformed/newer refusal.
- [Forced-interruption result](../../../Alloy-reference-records/deliverables/recovery-2026-10-14/api-01/result.json)
  and [transcript](../../../Alloy-reference-records/deliverables/recovery-2026-10-14/api-01/transcript.json):
  the normal Alloy executable was SIGKILLed after two dirty tabs, one named and
  one unnamed. Restart recovered exact authored IDs/parameters into separate
  pathless dirty tabs; malformed/newer records were refused, failed Save and
  unsaved tab close retained dirty state, both recovered saves succeeded, and
  the original named file hash was unchanged. This is local real-app API input,
  not a physical UI replay.
- [Recover Work dialog capture](../../../Alloy-reference-records/deliverables/recovery-2026-10-14/qt-06/recover-work-dialog.png)
  and [Qt log](../../../Alloy-reference-records/deliverables/recovery-2026-10-14/qt-06/ctest.log):
  production theme, active modal, actual hit-owned Recover button click, then
  recovered pathless dirty state. The close prompt's Save choice followed by a
  cancelled Save As left the tab visible and dirty; subsequent save succeeded.
  Owned-snapshot retention after cancellation is source-reviewed; the native
  test directly observes the retained dirty tab, not the hidden live snapshot.
  The test used `AA_DontUseNativeDialogs` so its `QFileDialog::reject` could
  cancel deterministically. macOS native Save-panel cancellation and physical
  input remain unverified. Earlier Qt01–05 harness attempts stalled at that
  native panel; they did not establish a product recovery failure.

The captured Qt06 row predates a source-only text polish that formats the
timestamp locally and uses singular “feature” when appropriate. The final
normal app relink passed after that wording change; no behavioral replay was
needed. Recovery is local best-effort crash protection, not a journal of every
draft interaction or a cross-machine backup.

Independent Astra source and evidence review accepted this bounded scope on
2026-10-14. No native macOS file-panel or physical-input claim is made.
