# FND-003 library comparison

Observed 2026-10-11, macOS arm64, Qt 6.11.2, OCCT 7.9.3.
This is a permitted library experiment, separate from Alloy's real application
and cumulative product gates. See [decision 001](../decisions/001-authored-document.md).

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/opt/homebrew -DALLOY_BUILD_SPIKES=ON
cmake --build build --target document_history_spike
build/document_history_spike build/evidence/FND-003
```

The successful comparison returned exit 0 and `passed` for both candidates.
The [public receipt](document-history.json) binds this observation to the exact
fixture source, executable and output bytes; generated documents remain in
ignored `build/evidence/FND-003/`. `ALLOY_BUILD_SPIKES` is off by default and
research code is not linked into the app or required by native compile CI.

| Dimension | Observation |
| --- | --- |
| Build | Optional native fixture compiled and linked |
| Application API / hardware / visual / physical input | Not run; no application workflow or new viewport claim |
| Library geometry | Valid box 15×20×40 mm; volume 12000 mm³; thin intent evaluated after load, independent BRep readback; OCAF named-shape readback |
| Authored persistence | Document/feature IDs, mm units, edited dimensions and a future empty-sketch record preserved |
| Thin transactions | Create undo/redo, grouped dimensions, cancel/invalid draft, saved checkpoint, divergent redo discard |
| OCAF transactions | Create undo/redo, two-dimension grouped edit, abort, divergent redo discard |
| Native format boundary | Thin newer-schema rejection; original preserved after child process exited before QSaveFile commit; failed-directory destination refused |
| Session journal | Neither candidate persisted session undo after reopen |
| CAD interchange / print export | Not run; BRep cache roundtrip is not STEP or printing export |

OCAF's application-schema validation and atomic replacement were not demonstrated;
those require application policy rather than being inferred from SaveAs. The
branch fixture tests journal discard, not recompute of the newly branched OCAF
width. No sketch solver, downstream dependency graph, persistent face/edge
tracking, asynchronous evaluation, recovery or power-loss claim is made.

Astra's independent read-only review passed: all seven recorded hashes matched,
library versions and results were consistent, and the decision/evidence boundaries
were accepted. No additional run was required.
