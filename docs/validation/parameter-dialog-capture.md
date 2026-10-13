# Owned parameter dialog control and capture (#250)

Candidate 2026-10-13, base4aebf9f; independent acceptance pending. The existing
Change Parameters dialog is now held by a safe owned pointer. Repeated open
reuses it; `document.parametersDialog {visible:false}` closes only that dialog
with normal close semantics. `capture.dialog` returns its actual widget pixels.
The existing owned-popup compositor already includes this dialog in the main
capture/movie; no new compositor or reconstructed controls were introduced.
The screenshot command description now matches that existing behavior.

Shared parameter mutations refresh table rows and preserve current selection by
stable parameter ID. The refresh blocks table signals and does not touch pending
name/value/unit inputs. An unchanged table is left alone. Closing the dialog
clears its owner pointer, allowing immediate reopen before deferred destruction.
No parameter schema, solver or geometry authority changed.

The isolated Debug app and existing named-parameter controls target built.
Only the new `ownedDialogCaptureAndPendingEdits` Qt case ran: 3 stages passed,
0 failures in2.165s. It checks repeated open, pending field text and selected ID
through three shared additions, row values, close/reopen and unchanged authored
state. These are widget-state checks, not a new native typing claim; accepted
#244 native table input remains separate and unchanged.

`tests/parameter_dialog_capture_api.py` passed through the real executable:
open, add Length75/Width50/Thickness10, actual dialog/main captures, short API
recording, close and exact authored comparison. External artifacts live in
`../Alloy-reference-records/deliverables/parameter-dialog-capture-2026-10-13/`.
The three-value table is readable in the actual captures; movie samples show
real evolving rows. OS decorations and surfaces outside the main capture bounds
remain excluded. This is not source styling parity or full plate acceptance.

The first Qt run stalled at its unsaved-document close prompt because test
cleanup omitted explicit discard. Root mistakenly launched the API test before
confirming that process had exited, briefly overlapping two private validation
processes. The owned stalled process was terminated; its incomplete log is
preserved. The test now explicitly discards its fixture before closing, and the
corrected run exits0. The API editor also exited0; no validation editor remains.
There was no application behavior change for this harness cleanup correction.
