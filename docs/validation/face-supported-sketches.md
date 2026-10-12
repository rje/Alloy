# Face-supported sketch checkpoint

Verified locally on macOS, 2026-10-12. The supported slice is the real end cap of
a positive-distance extrusion. Other planar faces, reversed extrusions and faces
modified by Boolean operations remain outside this slice.

A sketch retains producer/body/semantic-face UUIDs and local 2D coordinates.
Evaluation resolves the actual current cap and transports the source sketch frame
by extrusion depth. Width/radius edits retain the frame origin and identity;
depth edits move the frame without rewriting local geometry. Missing or unsupported
references fail atomically, with no origin-plane fallback.

## Evidence

- Combined application build passed in the isolated face-support worktree.
- Focused `face_support`, `extrusion_document` and `sketch_profiles` tests passed.
- Real-app `sketch-face-support` ledger passed at
  `build-face/evidence/face-support-ledger/report.json`, run `dzlcizyx`: actual
  rounded bracket, attached rectangle at z50, depth60 transport, retained local
  coordinates/profile/reference IDs, history, exact reopen and stale-reference refusal.
- Native top-face click → Create Sketch → R → two pointer clicks → Escape →
  Finish passed. Navigation and inspection used the API; modeling used native
  input. `face-native-{selected,rectangle,finished}.json` records authoritative
  states; captures show face selection and the rectangle on its supporting body.
  The saved native part is `build-face/evidence/native-supported-bracket.alloy`.
- Astra source review caught through-body face selection; the picker now checks
  the nearest real visible solid face before accepting a supported cap. Units and
  transitive descendant hiding were also corrected before the API/native run.

Evidence paths above are under `../Alloy-worktrees/face-support/`. Astra independently accepted the native captures and state: clear face highlight,
readable rectangle/profile against its support, exact draft-to-finished geometry,
and all world points at z50. The ledger executed 147 commands. No full tutorial, arbitrary topology naming or exact visual
parity is claimed. Projected model edges and their constraints are the next slice.
The private candidate was closed; the human testing application was unchanged.
