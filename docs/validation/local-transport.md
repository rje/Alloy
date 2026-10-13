# FND-024 local transport candidate

2026-10-13. Bounded child #220; parent #131 remains open. Source base e5f102f.
The application protocol and authoritative modeling commands remain unchanged.

## Contract

Discovery carries explicit transport/address through identity verification and
runner requests, retaining the Unix socket alias for older records. Windows app
names are fresh instance UUID pipes; explicit server override and remote pipe
names are refused. This target policy is an Alloy choice. Qt UserAccessOption is
retained. Native Windows registry/ready directory protection sets current-user
ownership and a protected full-access DACL inherited by files/subdirectories.
Existing policies are validated, including protection and inheritance; unknown,
shared or noninheritable policies refuse instead of being rewritten.

The console QtCore/Network byte bridge uses binary stdin/stdout, bounded newline
frames and one absolute connect/write/read deadline. Standard Python retains
JSON, response correlation and identity checks, kills/reaps on outer timeout,
and resolves the helper only from fixed package/build locations or explicit
caller configuration. Discovery cannot select executable paths. Existing Unix
socket inactivity timeouts are preserved. A timeout leaves command completion
unknown and does not cancel a command.

The helper and Python client are copied alongside future native milestone
artifacts. Standalone CMake needs Qt 6.11.2 Core/Network/Test and Python, without
OCCT or an editor. The new workflow is manual-only and has not been dispatched
by the implementer. It builds only helper/security/non-GUI tests on Windows.

## Observed evidence

- Standalone macOS build and focused CTest passed 1/1 in 3.22 seconds after review
  fixes. Actual QLocalServer/helper processes exercised fragmented exact bytes,
  malformed input, bounded input/output, partial EOF, connection/read timeout,
  successful Python request and exact wrong-ID refusal. Unix privacy validation
  passed. Windows pipe/ACL branches are not established by this local result.
- Existing client selection and new transport metadata selection tests passed.
  Discovery cannot supply a helper executable; transport is retained and remote
  Windows pipe syntax refuses. Python compilation passed.
- App and helper Debug targets built against the existing Qt 6.11.2 private
  runtime and OCCT SDK. One private macOS app regression passed and exited cleanly:
  ready-file discovery/live identity, forced Qt bridge, stale identity refusal,
  CLI selection, malformed server request, top orientation, search, framebuffer
  capture and quit. Capture was inspected and is readable. Blank authored design
  remained empty. No broader CAD or new hardware qualification is claimed.
- First real-app runner attempt had an incorrect CLI subcommand; corrected to
  `exec app.identity` and reran the bounded case. No product defect was inferred.

Evidence: `build-transport/Testing/Temporary/LastTest.log` and
`build-transport-app/evidence/local-control/{report.json,app.log,local-control.png}`.
Runner: `tests/local_control_api.py`. GUI slot released after clean exit.
Astra independently reviewed source and required protected/inheritable existing
ACLs, binary CRT streams and positive/exact correlation assertions; all corrected.

## Pending acceptance

Root must push/dispatch `.github/workflows/local-transport.yml` on the candidate
and retain its actual Windows report/log. Expected gate: real named-pipe byte
exchange, Windows owned-directory ACL validation/refusal and Python helper
correlation, without GUI. No actual Windows runtime claim yet. Parent #131 still
requires normal-user Windows application workflow, desktop/GPU and actual
second-user denial evidence. No general ACL manager or remote transport added.
