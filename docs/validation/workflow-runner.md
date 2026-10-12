# FND-010 workflow runner verification

Verified 2026-10-11 on macOS Apple silicon. The thin cumulative runner invokes
Alloy's existing real-entrypoint shell case with a hashed fixture. The final
configured build and FND-010 case passed. Live Qt6.11.2, OCCT build7.9.3 and Apple
M4 hardware renderer metadata were retained with source, binary and artifact hashes.

Focused checks rejected an omitted required capability, excluded shell case,
open/missing cumulative CAD prerequisites, and an arbitrary candidate path
presented as the configured build output. No new application/model state was
mocked. The [result record](workflow-runner.json) distinguishes final success
from earlier focused checks instead of relabeling their source/candidate hashes.
Astra independently reviewed the runner and those results; the identified
build/candidate binding and version-provenance issues were fixed and rechecked.

Build, API and hardware evidence passed for this bounded shell case. Geometry,
save/reopen and export are non-applicable to the shell; physical input was not
performed, and screenshots are capture evidence without visual-compatibility
acceptance. No CAD gate was closed. Only shell_api and FND-010 cases exist;
unimplemented cases and gate evidence specifications block acceptance.

See [usage and extension rules](../workflow-validation.md). The existing native
CTest shell case remains available. Further testing of unchanged behavior is
not needed for this task; new features add proportionate real cases.
