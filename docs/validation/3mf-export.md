# XCH-011: selected-body Core 3MF export

The accepted bounded slice adds `document.export3mf` to the local command API and
the existing Save As Mesh format sheet. It writes one Core mesh object and one
build item per selected committed solid, with millimeter units and each local
component occurrence placement encoded once. The native file chooser, async
completion/cancellation and atomic publication use the established mesh export
flow. Export does not edit the authored model, history, visibility or revision.
Materials, colors, optional 3MF extensions and 3MF import remain outside this
bounded export slice under XCH-005. Public format sources and dated provenance
are in `../Alloy-reference-records/knowledge/specs/xch-011-3mf-export-2026-10-17.md`.

The pinned lib3mf 2.5.0 source and submodule archives are recorded by SHA-256 in
`cmake/Lib3MF.cmake`. Their original notices are copied into the build's
`third-party-notices/lib3mf` and carried by the macOS packaging and CI artifact
scripts. The exact compiled libzip source notices are included because they
carry newer copyright years than the submodule's top-level LICENSE. This is
dependency admission and packaging wiring, not a
qualified released bundle.

Evidence from `build-preview-release`, app SHA-256
`4d11a746441e183b8861d1cdee1b5747894529cbb4496af2077b1543a4b92d7c`:

- `three_mf_writer_test` passed on detached OCCT bodies, including two boxes
  with a rotated/translated build item, analytic curved seams and publication
  refusal. Its output files are in `build-preview-release/evidence/xch011-core/`.
- `mesh_export_controls_test` passed the existing Qt cases, including Browser
  context entry, the 3MF option/unit-copy presentation, asynchronous
  publication and stale-result behavior. A focused additional
  `threeMfCommandCancellation` case passed the shared command's pending,
  cancellation, unchanged-destination and authored-state checks. Qt interaction
  evidence is distinct from a separate-process cancellation or physical input.
- The real application entrypoint passed
  `tests/three_mf_export_api.py` with one and two component instances, independent
  stdlib ZIP/XML geometry readback, 90-degree rotation plus translation, a
  source-width edit/re-export, and unchanged authored/undo/revision state during
  export. Source model, screenshot, 3MF files, transcript and parser receipt are
  under `../Alloy-reference-records/validation/xch011-3mf-2026-10-17-candidate1/`.
- A separate PrusaSlicer 2.9.6 CLI `--info` read the two-object 3MF and reported
  two manifold 12-facet parts at 20 x 10 x 5 mm and 10 x 20 x 5 mm respectively.
  Its output is `prusaslicer-info.txt` in the same external evidence directory.
  That CLI recenters each object's reported bounds; the independent XML readback
  establishes relative build placement.
- The external `../Alloy-reference-records/validation/xch011-3mf-2026-10-17-native/native-observation.md`
  record selected 3MF in the actual app sheet, cancelled the macOS chooser without
  starting an export, then saved a 3MF through that chooser. The output
  independently parsed as a 20 x 10 x 5 mm manifold box. No automatic video or
  physical hand-input qualification is claimed. The native chooser was observed
  through accessibility, without a saved capture of that popup; the main-window
  capture clips the second coupon and does not qualify full visual framing.

Astra independently reviewed the candidate's geometry, command dispatch,
readback and native observation. Its notice finding and cancellation-evidence
distinction were addressed in the source and this receipt.

This does not qualify Windows/Linux runtime, a downloadable/signable package,
visual parity to an external application, a slicer-generated toolpath, or Core
extensions. The 3MF consortium's current Core 1.4.0 revision and the public
rendered 1.3.0 source have not had a clause-by-clause diff; the tested Core mesh,
unit and build rules are the bounded claim.
