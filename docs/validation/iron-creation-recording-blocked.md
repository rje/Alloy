# Corrected iron from-blank recording blocked by UI-thread stalls

TUT-034 candidate13, 2026-10-17. This is a failed video gate with useful
functional evidence, not an accepted tutorial recording.

One private Alloy editor created the accepted corrected iron from blank through
the shared local commands: prepared reference canvases/sketches, mirrored
weight-40 nose, exterior and panel/grip surfaces, retained rear Trim trial with
exact Cancel/Undo, a bottom profile appended to the same closing Extrude, and
one selected Boundary Fill cell. The final evaluated solid's volume, area and
envelope match the independently accepted candidate12 exactly. The native
model and STL were saved. The planned post-movie source-edit replay did not run.

The run made 319 successful API calls across 44 recorded stages. Its sum of API
call time was 861.19 seconds. Expensive synchronous operations included the
bottom-profile preview at 119.4 seconds, its commit at 116.49 seconds, a final
Boundary Fill commit at 94.46 seconds and edit-dialog opening at 81.09 seconds.
`recording.stop` refused with `Frame gap exceeded 300 slots; refusing a
shortened clip`; the recorder had elapsed about 929 seconds but encoded only
about 503 seconds, with 193 delayed ticks. The runner exited nonzero and
closed its private editor. A cleanup retry returned `recording_inactive`, but
the original `recording_failed` remains the authoritative error.

External evidence is under
`deliverables/iron-reassessment-2026-10-17/candidate-13-from-blank-recording/`:
`README.md`, `receipt.json`, `long-command-timings.json`, `stage-ledger.json`,
the saved model/STL, transcript and failed recorder artifacts. This does not
replace the prior accepted [shape/solid milestone](iron-corrected-shape.md).
REL-033/#476 owns the concrete UI-thread blocking path; TUT-034/TUT-018 remain
open for a coherent accepted recording and gallery entry. Raising the recorder
gap limit alone would not make the editor responsive.
