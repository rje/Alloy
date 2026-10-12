# Cumulative real-app validation

FND-010 adds a stdlib Python runner around the existing real-entrypoint
`tests/shell_api.py`. It does not implement CAD or a separate automation model.
Only `shell_api` and `FND-010` cases exist today. The latter runs the shell plus
refusal checks using its actual observed capabilities.

```sh
python3 tests/workflows/run.py --case FND-010 --build --evidence build/evidence/FND-010
python3 tests/workflows/run.py --case shell_api --app /absolute/path/to/alloy --evidence build/evidence/shell
```

`--build` verifies the configured source root and requires its known app output,
then builds the `alloy` target. Custom candidate paths are tested as prebuilt.
Without `--build`, the report
identifies the prebuilt binary and marks compilation unperformed. Runtime
Qt version and compile-time OCCT version, host, commit/dirty state, actual commands and source/fixture/
artifact hashes identify the tested candidate. Each execution uses a fresh
subdirectory; `report.json` records its result without consuming older receipts.
Case scripts run with Python environment overrides disabled so optimization
cannot silently remove their assertions.

Exit codes are 0 for case success, 1 for a failed case and 2 for blocked acceptance.
Build, API, hardware, geometry, save/reopen, export, physical input and visual
results have separate statuses/reasons. A shell capture is recorded as captured;
physical inputs and visual compatibility have not been qualified by this runner.
CAD dimensions are non-applicable to the shell, not accepted modeling evidence.

To exercise refusal explicitly:

```sh
python3 tests/workflows/run.py --case FND-010 --omit-capability shell.commands --evidence build/evidence/missing-capability
python3 tests/workflows/run.py --case FND-010 --exclude-case shell_api --evidence build/evidence/missing-case
python3 tests/workflows/run.py --gate GATE-010 --cumulative --evidence build/evidence/gate-refusal
```

All three must exit 2 today. The later gate checks predecessor issue states
live through `gh`, an explicit registered gate evidence specification, case
coverage of every ancestor task, and inherited earlier
gate capabilities/evidence dimensions. Unknown cases and unavailable capabilities
block acceptance. Unauthenticated/offline gate prerequisite checks also block;
individual cases can run without GitHub access. A successful shell case cannot
close GATE-010, or requalify GATE-000's historical physical-input observation on a
new candidate. Gate reports never automatically close issues or publish releases.

The reviewed [ledger](../tests/workflows/ledger.json) associates cases with task
coverage and observed capabilities. The shell [fixture](../tests/workflows/fixtures/shell.json)
drives viewport sizes, view changes and command availability checks. Add actual
entrypoint cases and fixtures alongside each feature. Only a successful case
can contribute capabilities/coverage; names in the ledger are not test results.
Update the staged shell assertions as modeling becomes available and require new
geometry/undo/reopen/export fixtures. Do not keep asserting unavailable CAD as a
substitute for testing the implemented workflow. Physical/visual evidence needs
its own current, candidate-bound case before gates can accept it.
