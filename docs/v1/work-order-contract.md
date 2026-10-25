# V1 work-order and closure contract

Accepted planning method, 2026-10-25. This defines how to finish the selected v1
minimums; it does not implement them, change the scope freeze, or establish parity.
The canonical work orders and dependencies are in `../backlog/issues.json`.

## Before pulling a task

A feature work order names its starting model, supported source topology and
selection types, one successful result, one meaningful edit, and the material
refusal case. Resolve its interaction questions using the external knowledge
record before coding. A research prerequisite names the particular decisions
and evidence it must deliver. An issue with unresolved material behavior is
research-ready, not implementation-ready; keep the implementation dependent on
that decision. Do not ask the user to supply reference research.

For controls, pin the command name and entry route; preselection versus in-command
picking; add/replace/toggle/clear selection; field labels, types, units, initial
values and enabled/read-only states; shortcuts and focus; preview; OK/Apply versus
immediate commit; Cancel/Escape; and timeline edit. Use the source-specific
interaction, not a generic dialog for every command. An inspection or immediate
Delete command does not acquire a modeling panel just to fit a test harness.
A fixture value is not a documented product default. Explicit Alloy policies and
unknown parity remain labeled in the external command record.

The work order also names what is outside its minimum and the issue retaining
that work. A broad family parent stays open until its own remainder is done.
Release dependencies point to the selected minimum, not an accidentally broader
parent. Keep broad-parent-to-child links so the full roadmap still owns every
requirement. Do not remove requirements from accepted subsets to reduce counts.

## Feature closure

- UI and local control invoke the same authoritative commands, stable selection
  IDs and authored state. Proposed API IDs must be labeled until registered.
- The specified result has observable geometry or state assertions: counts,
  bounds/area/volume or analytic dimensions with stated tolerances, reference
  identity and relevant constraints. A screenshot cannot prove these facts.
- For preview commands, preview is disposable and does not publish history.
  Commit publishes one coherent transaction; Cancel and refused edits preserve
  the prior valid document. Immediate commands use their documented commit and
  Undo behavior. Selection/inspection alone must not edit authored geometry.
- Verify the stated source/parameter edit, Undo/Redo and source-independent native
  save/reopen where applicable. Use a single representative downstream feature
  when persistent references are part of the contract.
- Native controls are readable, enabled appropriately, highlight the real
  selection and display the evaluated result on the GPU. Follow actual focus
  and selection between inputs without harness repairs. Reuse existing captures
  and native evidence for unchanged controls; qualify new or changed interaction.
- Use focused unit tests for the changed core contract and a pragmatic real-app
  workflow. For a claimed printable result, independently read the representative
  export; inspection, navigation, research and packaging tasks do not each need
  their own printable mesh. Test the stated failure, not every possible kernel
  failure or option combination. Stop after sufficient evidence.
- Obtain bounded independent Sol review; reserve Astra for difficult geometry or
  larger cross-feature passes. Integrate accepted code and receipts, update the
  continuation docs, then close the issue and mark its board card Done.

Implementation, API/geometry, native input, visual presentation and packaged
platform evidence remain separate. A narrowly accepted implementation may close
while its explicitly linked native/platform qualification remains Todo. Do not
claim a native gate passes from simulated inputs, or leave a deferred child In
Progress. Do not close a parent merely because one child passes.

## Cumulative gates and reuse

A gate lists the required capability minimums and their concrete fixtures once.
It distinguishes runtime cases from reviewed research/license/CI receipts and
candidate-bound native/platform receipts. A `kind:validation` issue is not
necessarily a cumulative release gate. Unregistered proposed runner commands
are implementation dependencies, not executable acceptance instructions.

Reuse reviewed focused evidence when the relevant implementation, dependencies,
fixture and evidence contract remain applicable to the candidate. Record that
applicability explicitly; changed behavior or relevant platform risk requires a
rerun. Deduplicate shared fixture coverage. Do not replay every historical
workflow at each gate or satisfy missing coverage with empty aliases.

Actual extracted-package launch, local API, GPU/framebuffer/resize/DPI, native
input and representative save/reopen/export are still qualified on every
advertised OS against exact package hashes. Reused geometry receipts cannot
stand in for missing Windows/Linux runtime or physical input evidence. Authorized
limitations must be explicit. OpenGL may qualify for v1; Metal/Vulkan migration
remains later work. A successful planning gate never authorizes paid CI, a
build-triggering tag, signing, or publication.

## Keeping the burndown honest

An issue's exit is its stated deliverable, not a generic “all parity” claim.
Split a large owner into explicit implementation, native qualification or
research decisions when these have independently finishable results. Preserve
all remainders and exact dependencies. Use the Release scope field derived from
REL-054; In Progress means currently active work. Update the scope sync after
publishing changes. Counts measure work decomposition, not product completion.
