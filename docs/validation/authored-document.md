# DOC-002 authored ownership and units

Observed 2026-10-11, Qt 6.11.2 / OCCT 7.9.3, native macOS arm64 Debug build.
Application integration now uses `CadDocument` as the authoritative authored
state. It follows [decision 001](../decisions/001-authored-document.md).

`cmake --build build --target alloy document_model_test` passed.
`build/document_model_test` passed ownership, UUID lifetime, duplicate refusal,
1 inch = 25.4 mm, display-unit edits without parameter rescaling and undo identity.
These are core document tests, not a rendered modeling workflow.

`python3 tests/workflows/run.py --case DOC-002 --evidence build/evidence/DOC-002`
passed through the actual Alloy entrypoint. It inspected empty authored state,
changed display units, preserved the document UUID, rejected unsupported units
and an unacknowledged dirty replacement, then explicitly created a new identity.
`ctest --test-dir build -R '^shell_api$' --output-on-failure` also passed after the
startup contract changed from no document to an empty authored document.

[authored-document.json](authored-document.json) binds source and candidate bytes
to the generated runner receipt and unit-test binary. No unchanged workflow was
rerun after these checks passed. API ownership inspection exposes the authored
root and canonical values directly; it does not derive IDs from Browser labels.

There are no user-creatable modeled bodies yet. Typed box intent is
validated in the core fixture; cylinder validation is implemented but unexercised
at this stage. These prepare DOC-007 and have not been rendered
or published as a modeling capability. Body count remains zero until actual
evaluation is integrated. Sketch types are not admitted by the application
schema. Native storage, geometry summaries and modeling sessions remain open.
The core has one Qt journal, but history UI/API commands and editing-session
contracts are separate DOC-003/FND-006 work.

Astra independently checked the implementation, all recorded hashes and evidence;
no blocker remained. The runner help and descriptive limits were updated after
the recorded run to list DOC-002; its receipt retains the actual executed bytes.
Those text changes did not require another app run.
