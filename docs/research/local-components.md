# Bounded local component contract (ASM-004)

Public source ID `tutorial-library-die`, retrieved 2026-10-13. The source ledger
and original URLs remain external in
`../Alloy-reference-records/knowledge/tutorials/warwick/library-die-ledger.md`.
Documented instructions name a component before modeling and use a Browser
radio control to activate the desired component before print export. Public
component documentation says new sketches, bodies and construction geometry
belong to the active component. Creating the component before modeling keeps
its history together. The source does not establish an Activate checkbox default
or reopen activation behavior, nor provide a matched final die screenshot.

Alloy's bounded choices: New Component defaults its Activate checkbox to checked;
reopen activates root. The schema separates a local definition `{id,name}` from
its distinct root-child occurrence `{id,definitionId,parentId,placement:"identity"}`.
Root is the implicit document definition. Optional top-level `components` and
`occurrences` arrays preserve root-only legacy documents. Each child definition
requires exactly one occurrence. Nesting, reused instances, transforms, external
references and joints remain outside this increment and in ASM-001.

New Component is available in Solid Create and the shared `component.newDialog`
command. Name and Activate feed `component.create {name,activate?}`. Each design
has an independent active authoring definition, selected by Browser radio or
`component.activate {definitionId}`. Activation changes neither authored data,
revision, dirty state nor CAD undo. Undo removing the active definition falls back
to root; redo restores authored identities without reactivating it. Browser groups
real bodies, sketches and construction under their owner. Timeline shows the
active definition's features; viewport continues to show all visible bodies.
New geometry uses the active definition, while edits preserve original ownership.
Unsupported cross-component modeling inputs fail explicitly before publication.

`component.inspect` and `document.inspect` expose definition/occurrence/active
owner IDs; actual evaluated bodies, sketches and planes expose `ownerId`.
Save As Mesh offers `Component: <name>` in its existing selector and initially
selects the active child when no explicit body selection was supplied. Shared
`document.exportStl {occurrenceId,...}` resolves every actual material body owned
by that definition, including hidden material bodies. It excludes root stock and
other definitions. Empty and stale scopes fail; units and existing refinement
controls remain explicit. Selection by component name or visibility is not used.
Export neither creates CAD history nor changes placement.

## Verification gate

Focused `local_components` covers root stock plus a named child, centered
20 mm sketch/extrusion/all-edge fillet, activation/history, editing from root,
invalid ownership/occurrences, atomic cross-owner refusal, exact native storage,
and independently read-back 20 mm STL bounds excluding 100 mm root stock.
`tests/local_components_api.py` provides the cumulative real-entrypoint workflow,
with separate manifold connected-mesh readback, per-tab activation, native Browser
capture and actual export-dialog state. Run only in the serialized desktop slot.

Independent native review must inspect New Component Name/Activate/OK/Cancel,
root/child Browser radio activation, actual ownership groups, timeline scope,
component export selection, save/reopen and geometry. Match to documented
workflow; visual comparison remains unknown without a matched public capture.
This contract and focused tests alone do not establish native acceptance.
