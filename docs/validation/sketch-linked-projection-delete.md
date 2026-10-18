# SKT-143: linked Project geometry Delete

Accepted bounded slice, 2026-10-18. Independent Astra review accepted the
geometry, history, native routes and evidence; its visual finding was corrected
and the final native captures were inspected afterward. `sketch.deleteSelection` now removes a selected
retained straight, circular or point projection through the shared
`SketchGeometry::deleteEntities` command. The retained local row and virtual
endpoints/center disappear; source bodies, support references, unrelated
projection rows and authored geometry retain their identities. The native
selection list now includes evaluated projection geometry using those same
stable IDs. Straight/circular curves use `entityIds`; projected points use
`pointIds`. There are no new options, defaults, preview or confirmation.

This is **Alloy policy**, not a demonstrated reference-product live Delete
cascade. Permitted public evidence, retrieval dates, distinctions between
geometry deletion and Break Link, and the bounded interaction contract are in
external knowledge record
`../Alloy-reference-records/knowledge/specs/skt143-linked-projected-delete-2026-10-18.md`
(S1–S6). Its live-selection cascade, annotation cascade, keyboard/context
semantics and downstream behavior remain unknown. No proprietary application
was operated or inspected. The app remains Alloy-branded.

## Ownership and failure policy

The command explicitly recognizes the three retained projection arrays and
retires only selected rows before checking surviving metadata references. It
does not strip arbitrary JSON, delete upstream sources or implement Break Link.
Current source context must still resolve: lost-link repair is separate work.
A surviving authored entity that uses a removed virtual endpoint, circle center
or projected point causes atomic refusal naming the entity. Surviving retained
operations that consume a selected projection also refuse; a Circle Offset
source is one tested example. Directly attached constraints and dimensions
retire as annotations, consistent with Alloy's existing ordinary Delete policy.
Unrelated relations survive. Intrinsic projected-circle diameter presentation
is synthesized from its retained source, rather than stored as an authored
measurement or independent Delete target.

Duplicate, stale and wrong-kind IDs refuse before publication. Failed solve
feedback may update `lastSolveFailure`; the draft, selection, draft history,
projection context and committed document remain exact. Successful deletion
adds one draft history step. Historical selected-profile preflight still runs
on the solved private candidate and refuses deletion required by a committed
Extrude before changing the draft or document.

Projection removal exposed a history bug: Undo resolved the restored context,
then evaluated using the previous session's context, where the deleted row was
absent. History evaluation now uses its freshly resolved context before
publishing either Undo or Redo. Ordinary sketch history was separately checked.

## Verification

Inspected `build-preview-release/CMakeCache.txt`: native Release, Qt 6.11.2
private runtime. Built `alloy`, `sketch_linked_projection_delete_test`,
`sketch_constructor_delete_test` and `linked_projection_delete_controls_test`.
The focused core test passes all three owner kinds, exact unrelated authored
parameters, attached circle measurement cleanup, stale/duplicate/wrong-kind
refusals, direct virtual-point entity dependency refusals, and retained Circle
Offset source refusal. The existing constructor/Mirror/Offset Delete core
regression passes.

`tests/sketch_linked_projection_delete_api.py` passes through the actual app
entrypoint. It reuses the accepted occurrence-backed circular Project source
fixture, plus two small authored boxes for the straight loop and isolated
projected point. The fixture keeps a separate authored circle and retained
measurement. For each projected kind, it checks exact expected parameter JSON,
one-step history, exact Undo/Redo and restored resolved projection context.
It also checks atomic stale/duplicate/wrong-kind requests and a surviving Circle
Offset dependency. A second committed state consumes the 300 mm² projected
rectangle profile in an Extrude: deleting its line refuses with the named
consumer/profile and leaves draft/history/selection/document exact; deleting
an unrelated projected point succeeds and Finish/save/reopen preserve the exact
solid bodies. Saved authored JSON is exact after reopening.

`linked_projection_delete_controls_test` passes Qt input to the actual widgets:
line canvas click/Delete, circle canvas context/Delete, and projected point
list click/Backspace. It observes the selected stable IDs and actual focus
before keys, popup activation/closure around context-menu input, exact history,
and saved/reopened authored data. Initial macOS harness activation is separate;
there is no focus/selection repair between demonstrated steps. The list click
checks for an embedded receiving control before delivery to its viewport.
The existing nonfatal popup top-level-window warning remains. This is native
widget evidence, not physical hand input.

`tests/sketch_circle_offset_delete_api.py` also passes ordinary source/result
and association deletion, exact Undo/Redo and save/reopen against the same
final application, covering the shared history change.

Evidence root:
`../Alloy-reference-records/validation/sketch-linked-projection-delete-2026-10-18/`.
`api/receipt.json` records the modeling candidate app SHA-256, stable IDs, resolved projection
context, source-body summaries and downstream refusal. `api/transcript.json`
retains the executed commands and checks. `native/receipt.json` retains exact
before/after authored fixtures and route assertions; `ordinary-regression/`
contains the separate ordinary-history receipt. Selected/deleted/undone/refused
and reopened PNGs come from Alloy's native GPU framebuffer capture.

API and native GPU receipts independently report Apple vendor, Apple M4,
OCCT OpenGL, `4.1 Metal - 90.5`, hardware acceleration enabled. Actual captures
were inspected: projected boundaries retain purple presentation, the selected
boundary disappears after Delete, unrelated source/support remain, and the
point row is selectable. Broad framing includes the imported source assembly;
small circular geometry is correspondingly small. No matched public Delete
screenshot exists, so spacing, selection colors and exact reference styling
remain unknown. No resize/display-scale qualification is claimed by this slice.

## Bounded remaining work

All three current retained kinds have real-app/native evidence. Face projection
multi-boundary ownership, unsupported analytic curves, independent copies,
broken-source management, relink/Break Link and broader downstream consumer
families remain outside this slice. Explicit coincident authored/projected
curve input disambiguation was not qualified here: the list retains separate
stable curve IDs, but the fixture does not demonstrate that overlapping case.
Physical hand input and Windows/Linux runtime/GPU qualification remain separate
release gates. This receipt does not establish overall exact compatibility.

## Independent visual review correction

The independent review found overlapping orange profile diagnostics and the
selection helper in `native/native-line-deleted.png`. Native widget geometry
confirmed that the warning label had the correct 45-pixel height for three
15-pixel text lines, while the following helper retained its previous position:
label y113–157, helper beginning at y149. Increasing the label minimum height
resized that widget before Qt applied the pending layout to its siblings.
The correction activates its parent layout immediately when the measured
minimum height changes; routine refreshes with unchanged height do not force
layout work. The sizing policy and warning text are unchanged.

The final native fixture asserts text fit and helper separation in the same
refresh after Delete, repeats the assertion after Undo restores the closed
profile, and checks a separate ordinary one-region circle sketch. Captures at
the original 3000×1900 Retina pixel size (1500×950 logical window; the review
viewer scales these to approximately 1970×1245) retain the deleted and restored
projection states
and `native-ordinary-profile.png`. API/core modeling receipts precede this
presentation-only correction; the final native receipt records the corrected
application and native-test hashes separately.

The corrected native gate passes all three Delete routes, exact history/reopen
and both palette checks. Inspected the deleted and ordinary captures: diagnostic
text and the helper now occupy separate readable rows. Final application
SHA-256: `3e9794d7a98bcf7edf5cfa706b952d2d1134b7892da6b36f0557592fec0791a8`.
The final native test SHA-256 and exact capture dimensions are in
`native/receipt.json`. The source correction only changes palette layout; the
earlier API/core and ordinary-history receipts retain their original binary
hash and are not relabeled as runs of the corrected binary.
