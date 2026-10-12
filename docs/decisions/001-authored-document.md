# Authored document, transactions and native storage

Decision 2026-10-11, FND-003. Tested Qt 6.11.2 and OCCT 7.9.3 on macOS arm64.
The [prototype receipt](../validation/document-history.md) records the experiment;
this decision does not claim these systems exist in the application yet.

## Selected approach

Use a versioned, readable Alloy JSON document for authored intent and one
`QUndoStack` per open document as the sole editing-transaction authority.
Keep evaluated OCCT shapes and viewport objects in revision-associated caches.
They are never the source of editable dimensions, feature identity or units.
Use `QSaveFile` with direct-write fallback disabled for native atomic replacement.
OCAF is not an additional transaction stack or a required native file container.
Its public naming/history tools may be evaluated independently in FND-004.

The first document schema has a stable UUID for the document and each authored
feature/body, explicit root ownership, display units (`mm` or `in`) and canonical
millimeter geometry. A feature retains its type, name, visibility, typed
parameters, stable input references and dependency order as needed by implemented
features. Empty sketches in the experiment are future intent records, not solved
sketches. Do not admit types that the application cannot evaluate.

For the small first model, a transaction stores implicitly shared before/after
authored values. Only a fully validated candidate is pushed, once per user
commit. Preview and cancel use a disposable draft. Never push undo entries for
preview updates, validation failure or a no-op. A grouped change is one validated
replacement, so an intermediate failure cannot partially apply a macro.
Later large-model profiling can justify feature deltas; the persisted schema
does not depend on the in-memory journal representation.

Undo/redo restores authored values and IDs; it increments an application revision
and reevaluates the applicable intent. A new committed edit after undo drops the
redo branch. The clean checkpoint tracks a successful save; undo/redo to that
checkpoint restores clean state. Native reopen restores the editable feature
history with a fresh, empty, clean **session undo stack**. Session undo is not
serialized in schema 1. File paths, selection and camera are not modeling undo.

## Why this choice

Both candidates ran successfully. OCAF can commit, abort, undo, redo and persist
authored attributes plus named BRep results. It requires an application attribute
schema, label ownership, a validation layer and application-controlled save
policy. The fixture encountered an ownership error when named-shape attribute
handles survived document closure; releasing those handles fixed it. That is a
prototype integration finding, not a claim that OCAF is unreliable.

The thin candidate directly expresses Alloy's authored parameters and can be
inspected or modified without an OCAF container or custom attribute drivers.
Qt already supplies the required journal and clean checkpoint. In this fixture,
authored JSON was 500 bytes and OCAF intent plus evaluated BRep was 4116 bytes.
These carry different data; the sizes are not a general performance comparison.
No CAD correctness or persistent-reference advantage is inferred from file size.

## Evaluation and reference boundaries

Validate schema, ownership, units, IDs, dependencies and feature parameters
before replacing live intent. For the first synchronous primitives, evaluate a
complete candidate before commit; a kernel failure leaves current intent and
geometry untouched. Later dependency chains require per-feature errors and
explicit stale last-valid results, with failed downstream consumption blocked.
Results carry the authored revision; future asynchronous jobs must reject an
obsolete result rather than attach it to newer intent. There is no async scheduler
in this decision or prototype.

Semantic primitive references and OCCT Generated/Modified/IsDeleted history are
the FND-004 investigation. This decision does not solve topology identity, split
or merge, extrusion/fillet recompute, undo performance or crash recovery.

## Native storage and migrations

Schema 1 is `alloy-document/1`. Decode and validate a complete temporary
document before swapping it into the window. Reject malformed or newer schemas
without changing the current document. Future migrations are explicit functions
from a known prior schema to the current authored schema, with bounded fixtures;
never silently reinterpret unknown feature types or units. Keep the prior file
until a successful atomic commit; clear dirty only after that commit succeeds.

The real pre-replacement process-exit fixture preserved the original JSON bytes.
This proves that bounded failure path, not power-loss durability, filesystem
transactionality across several files or a recovery/autosave implementation.
Optional shape caches are regenerable and should not enter the initial format.

## Implementation follow-ups

- DOC-002: typed ownership, UUIDs, units and authoritative authored inspection.
- FND-006 / DOC-003: shared command sessions and one validated undo transaction.
- DOC-004: schema validation, atomic save and reopen without losing current work.
- DOC-001 / DOC-007 / DOC-008: evaluate and inspect real primitives before commit.
- FND-004 / DOC-012: bounded, explicit persistent geometric-reference policy.

Public foundation references retrieved 2026-10-11:
[Qt 6.11 QUndoStack](https://doc.qt.io/qt-6.11/qundostack.html),
[Qt 6.11 QSaveFile](https://doc.qt.io/qt-6.11/qsavefile.html), and
[OCCT OCAF guide](https://occt3d.com/dev/doc/overview/html/occt_user_guides__ocaf.html).
The OCAF guide is living documentation; the fixture used installed 7.9.3 public
headers and libraries. No proprietary application was operated or inspected.
