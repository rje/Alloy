# Section Analysis implementation gate

Prepared 2026-10-13 for DOC-021/#262. This is a proposed implementation contract,
not runtime evidence. Source workflow: external ornament-pattern knowledge record.

## Rendering foundation

Use OCCT `Graphic3d_ClipPlane` attached to solid presentations, with capping and
hatching enabled. Its plane is expressed in world coordinates. Enable inherited
object material for the source's component-color role. Keep sketch, origin-plane
selection cues and UI decorations outside the clipped solid presentation set.
The existing `ReferenceView` owns presentation lifetime; rebuild, document switch
and analysis visibility changes must reapply or remove the active plane there.

The installed OCCT headers expose `SetCapping`, `SetCappingHatch`, and
`SetUseObjectMaterial`; the [official reference manual](https://www.occt3d.com/dev/doc/refman/html/class_graphic3d___clip_plane.html)
documents these controls (retrieved 2026-10-13). Header availability is not proof
that capping renders correctly in Alloy's current OpenGL configuration. Prove a
hatched hollow-solid section on the actual GPU before expanding UI plumbing.

## Proposed retained contract

Store analysis records outside the modeling feature array, in an optional
document-owned `analyses` array. Each record has stable ID, name, origin-plane
identity, visibility and hatch state. Initially support XY/XZ/YZ at zero offset
and angle. Do not display nonfunctional transform controls. Choose and disclose
the retained half-space direction; the source does not establish its default.
Use a documented fixed normal for each origin plane, or persist the direction;
reopening must never choose a different half-space from the camera position.

Creation/edit/visibility use the existing document undo authority and atomic
save. Analysis actions may add undo entries, but must not add modeling timeline
features, change geometry, or alter feature dependencies. Preview is session
state only; Cancel restores the previous presentation and exact authored state.
Only one section is active at a time in this bounded implementation; selecting
another must explicitly deactivate the previous one. This is an Alloy policy,
not an observed source limitation.
While previewing a replacement analysis, suppress the previously active section
transiently. Cancel must restore it; accepting the replacement deactivates it in
the same undo entry as the new analysis.

Validate the new schema during document open, including document-wide duplicate IDs, valid
origin planes, and the single-active rule. Existing files without analyses remain
valid. Ordinary model commits must preserve analysis records. Use existing
document transactions first; avoid a second history stack or separate save file.
The existing candidate evaluator re-solves sketches. An analysis-only transaction
must preserve the modeling JSON and evaluated geometry exactly: reuse the current
evaluated state through the existing undo authority after validating the analysis
change, or demonstrate exact preservation before using general recomputation.
Do not let an inspection toggle normalize authored sketch coordinates.

## Acceptance evidence

1. Actual GPU proof: hollow solid, oblique camera, clipped cavity, hatched material
   cross-section and no filled cavity. Hatching off/on and analysis hide restore
   distinguishable expected views. Capture at readable scale.
2. Shared commands and native Inspect command: choose an origin plane, preview,
   Cancel, create, edit, hide/show. Match the external reference's section roles.
   No focus repairs during the native sequence.
3. Geometry and features stay identical through inspection; one edit undo/redo
   restores exact analysis state. Save/reopen and document-tab switching restore
   the correct document's analysis without leaking clipping into another design.
4. Independent Astra review of native and GPU evidence. The later full ornament
   gate repeats the source section step on the actual revolved shell.

General transformed planes, multiple simultaneous sections, measurements and
other Inspect tools remain in DOC-014. They are not claimed by this gate.

Astra reviewed this proposed contract on 2026-10-13. The atomic replacement,
evaluation-preservation and ID rules above incorporate that review. No GPU or
native implementation acceptance has occurred.

## Isolated GPU feasibility result

Root ran an external fixture linked to the existing `alloy_application` library,
using the actual `ReferenceView` and a kernel-created hollow cylinder. This is
not the application command entrypoint, native input, or the ornament tutorial.
The fixture temporarily accesses renderer internals; that access is not proposed
production architecture. Both private proof processes exited normally.

Built-in `SetCappingHatch` yielded identical hatched/solid-cap images. OCCT 7.9.3
[OpenGl_Context.cxx](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/OpenGl/OpenGl_Context.cxx)
(retrieved 2026-10-13) disables polygon stipple without fixed-function support.
The observed Mac core-profile result is consistent with that limitation.

An independently generated repeating 32×32 diagonal-stripe texture, projected
onto the cap through `Graphic3d_Texture2Dplane`, visibly hatches both side walls
and the base cross-section while preserving the open cavity and inherited body
material. Hatched, plain-cap and unclipped captures are distinct. Production
should use this capping texture path rather than relying on legacy stipple.
Hatch spacing/style still needs comparison with the source inspection image.

External evidence: `validation/section-analysis/proof/` contains source, executable,
three captures, GPU diagnostics and report; `attempt-01-stipple/` preserves the
failed built-in-hatch experiment. Astra independently inspected the three images,
source and GPU diagnostics and accepted bounded Apple M4 / OpenGL 4.1 Metal
rendering feasibility. Derive texture axes from each chosen plane in production;
the fixed X/Z axes here exercise only one plane. Reapply clipping when solid
actors are recreated. Other materials and platforms remain unverified.
Retained analysis records, shared commands and native controls are not implemented.
