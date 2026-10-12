# FND-006 command sessions and live discovery

Observed 2026-10-11, native macOS arm64 Debug app, Qt 6.11.2 / OCCT 7.9.3.
[command-sessions.json](command-sessions.json) records source/candidate/test hashes.

`python3 tests/workflows/run.py --case FND-006 --evidence build/evidence/FND-006`
passed. Two real Alloy processes demonstrated live process/window/instance
verification, explicit multi-instance choice, stale-record refusal before sending
an edit, and recovery to the remaining live instance after process exit.

The display-units dialog and API share one command session. Input updates changed
the real QComboBox while leaving authored state/journal unchanged. Preview
inspected a disposable authored draft; commit applied one document transaction
with a stable transaction ID. Cancel and invalid inputs preserved authored state,
cancelled sessions could not commit, and a changed document caused stale-commit
refusal. Search uses the same lifecycle with an observable actual query widget.
Disconnecting an incomplete request applied no command. Responses retained
request correlation and explicit completion/document revisions.

The actual settings QWidget was captured via `capture.dialog`. It is separate
from hardware viewport/compositor evidence. No geometry is implied by the units
preview. Future modeling sessions are added with their real features, and async
jobs/cancellation wait for a real long operation. Windows named-pipe/startup
adaptation remains FND-012; macOS/Linux protocol design does not qualify Windows.

The first run exposed a Qt JSON read accidentally inserting an empty-session
field. Reads now use `value()`; the affected FND-006 workflow and shell regression
both passed after the fix. The native document core test also passed. No further
unchanged app runs are required for this slice.

Astra found an explicit instance selector could be overridden by ALLOY_SOCKET.
Selectors are now exclusive, and an explicit UUID overrides environment fallback.
The focused metadata-only client regression passed; this did not require another
two-app run. The original workflow retains its executed client hash; the receipt
also records the corrected client and regression hashes.
