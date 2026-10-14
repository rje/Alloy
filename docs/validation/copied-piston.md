# Copied piston mechanism checkpoint

ASM-016/#365 is independently accepted at `560ccf1` (authority `3de725e`,
reviewed sources `9416c1e` and `90a80a6`). The adapted six-part fixture now has
five Revolute joints, two Cylindrical joints and the original Lower arm Pin.

Copy retains shared source definitions and body/edge identities, creates new
occurrence IDs, and remaps only the internal Cylindrical joint. External eye
joints and Pin remain unchanged. The copy command adds `jointMap` to its existing
`occurrenceMap` result. Copy uses the reference rest pose; retained nonzero joint
coordinates and transient staged motion explicitly refuse. Revert clears a
preview; it does not rewrite a committed hinge angle.

Joint Position retains occurrence identity. Component selectors distinguish
instances, and the same Upper arm drag command solves all six leaves. Composed
authored rest transforms seed the existing solver; original joint and full Pin
checks remain in place. No new solver ABI or per-part animation was introduced.

## Evidence

- Actual seven-row core checks signed motion, original relations and Pin, six
  placed bodies, a shared source height edit, coordinate/source-edit history and
  reopen. Existing coupled and unjointed-copy checks pass.
- `tests/copied_piston_api.py` passes 36 real-app calls. At -15 degrees the right
  piston travels -21.9717 mm and the left +25.0127 mm; at +15 degrees these swap.
  Independent mounting-position and length calculations check both pistons.
  Authored state stays unchanged during preview, Revert restores every occurrence,
  and the committed angle survives undo/redo and exact authored reopen.
- Focused native test passes 3/3 in 47.78 seconds: staged Copy refusal, typed
  -180 mm placement, an actual copied circular-edge Qt click and Revolute OK, then
  Upper arm drag on the completed mechanism. Five leaf poses change; Revert
  restores the baseline. Component-selector setup is programmatic. This is Qt
  input evidence, not a complete physical OS-input walkthrough.
- The final shortened component labels were source/build checked after the
  native capture. Source/API/native GPU captures were inspected independently.
  Copy-specific undo uses the established whole-document transaction and prior
  ASM-017 evidence; this increment directly checks coordinate/source-edit history.
- Integrated main app builds; copied assembly and linked/export core checks pass
  2/2 in 2.78 seconds.

Evidence is external under `deliverables/copied-piston-2026-10-14/`.

This verifies sampled kinematics of independently authored tutorial geometry.
It does not certify collision/contact, physical fit, stroke limits, continuous
performance, independent graph-rank analysis or exact source sample geometry.
Per-joint relative DOFs and driver/passive coordinates remain inspectable.
The independently accepted cumulative API movie is external under
`deliverables/copied-piston-2026-10-14/from-blank-01/`, linked at gallery
`#piston-full-output`: 351.667 seconds, 1224×902 H264 at 15 fps, 5275 frames,
4844 repeated frames and zero capture failures. One real application creates the
parts, builds the original mechanism, copies/solves both pistons, then exports
and reimports placed geometry. All four stage receipts pass. Raw computation
waits remain; small sketch details are not close-ups. API command overlays are
not physical-input evidence. The raw recorder's `inputScope` denotes capability,
not observed input in this movie.

The exact native source-step ledger preserves ASM-019–023 for Copy/Paste,
locked origins/visibility, drag placement/contextual Revert, Browser gestures,
and shared As-Built command selection. TUT-019 remains open until these pass.
