# Failed native save: bounded REL-055 receipt

2026-10-19, macOS 26.5.1 arm64. Core regression passed; real-entrypoint API
qualification remains pending. No native input, screenshot or cross-platform
runtime acceptance is claimed.

## Contract and implementation

Replacing an existing `.alloy` document must fail without damaging the last
valid file, clearing the current dirty state, changing authored identities or
altering Undo/Redo. After removing the failure condition, the same document must
save and reopen successfully.

The existing `CadDocument::save` already uses `QSaveFile` with direct-write
fallback explicitly disabled, and updates the path/clean checkpoint only after
the complete write and successful commit. No product change was needed.
`document.save` already maps a storage failure to `ok: false`,
`completion: failed`, error code `save_failed` and field `path`; its native Save
actions invoke this shared command. This structured response is source-reviewed
here; the new real-app driver has not yet executed it.

This is an Alloy integrity requirement from REL-055, with no new controls or
presentation. No proprietary runtime observation or exact reference behavior is
claimed. The external knowledge search found no additional failed-replacement
contract needed for this unchanged Save workflow.

## Core evidence

`tests/document_model.cpp` extends the existing authored-document test. It saves
one authored 25.4 × 20 × 10 mm box (5080 mm³), changes its width to 30.4 mm,
and retains both an undoable geometry edit and a redoable units edit. The test:

- Leaves the destination file writable and restricts its directory to owner
  read/execute, so temporary replacement creation fails. It separately verifies
  creation is refused; a privileged run cannot silently satisfy the fixture.
- Calls the real `CadDocument::save` on that existing path and requires a failure.
  The exact document state, authored JSON, IDs, path and Undo/Redo remain intact
  and dirty. The original file remains byte-identical.
- Opens that actual original path into another `CadDocument`, checks exact
  authored data and original evaluated volume, then restores directory access.
- Retries Save, checks the complete edited JSON and clean state, reopens it, and
  exercises Undo/Redo back to the saved checkpoint.

Permission restoration uses a scope guard, including assertion failures.
The permission fixture runs on Unix; Windows reports it as unqualified and
continues the pre-existing document assertions.

Executed on the shared main checkout based on `c003e80`:

```sh
cmake --build build-preview-release --target document_model_test -j4
ctest --test-dir build-preview-release -R '^document_model$' --output-on-failure
python3 -m py_compile tests/failed_save_api.py
```

Build: passed. Test: **1/1 passed**, 0.52 s total. Python syntax: passed.
The inspected cache uses Ninja/Release and Qt6_DIR
`/Users/rje/src/cpp/Alloy-dependencies/qt-private-runtime/install/lib/cmake/Qt6`
(Qt 6.11.2); the accepted kernel is OCCT 7.9.3. No CI dispatch or GUI launch.

## Pending actual-application check

`tests/failed_save_api.py` uses the existing native entrypoint/local control
harness. It creates a 20 × 30 × 10 mm box, saves, edits width to 24 mm and
retains Undo/Redo before applying the same permission failure. It asserts the
structured failure, unchanged full document inspection and prior bytes, then
checks successful retry, Undo/Redo, original-byte snapshot reopen and edited
file reopen. It writes `previous.alloy`, `retry.alloy`, `app.log` and a JSON
response transcript with both file hashes.

Run only when the single private GUI lease is free and the Mac is unlocked:

```sh
python3 tests/failed_save_api.py \
  --app build-preview-release/alloy.app/Contents/MacOS/alloy \
  --output ../Alloy-reference-records/validation/failed-save-2026-10-19/api
```

The application target must be rebuilt before this pending run if its binary is
stale. This driver is syntax-checked only; its results must not be presented as
executed API evidence. Independent compatibility review also remains pending.

## Limits

The deterministic failure is refusal to create a temporary replacement beside
an existing writable native file. It does not simulate interrupted large writes,
late commit/rename failure, disk exhaustion, crash durability or older-schema
migration. Those broader requirements remain with REL-002. Native chooser,
physical input, GPU/visual state and Windows/Linux runtime are outside this
core fixture. No visual changes were made.
