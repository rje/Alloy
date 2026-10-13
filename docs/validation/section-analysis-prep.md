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

Creation/edit/visibility use the existing document undo authority and atomic
save. Analysis actions may add undo entries, but must not add modeling timeline
features, change geometry, or alter feature dependencies. Preview is session
state only; Cancel restores the previous presentation and exact authored state.
Only one section is active at a time in this bounded implementation; selecting
another must explicitly deactivate the previous one. This is an Alloy policy,
not an observed source limitation.

Validate the new schema during document open, including duplicate IDs, valid
origin planes, and the single-active rule. Existing files without analyses remain
valid. Ordinary model commits must preserve analysis records. Use existing
document transactions first; avoid a second history stack or separate save file.

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
