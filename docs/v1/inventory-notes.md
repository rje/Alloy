# V1 command and capability inventory

[command-inventory.json](command-inventory.json) is a review proposal for the
standalone shape-creation and 3D-printing release. It covers existing tools,
missing tools/options, release gates and explicit later families. It does not
reduce the long-term product contract or approve any deferral.

The inventory has **397 exact registered command IDs**: 396 available entries
and the unavailable `inspect.measure` placeholder. Its **150 readable rows**
combine related controls/aliases and expose missing capabilities. Counts describe
inventory structure, not percentage complete. `present` means the named bounded
authority exists; `partial` retains material limits; neither means v1 ready.
Native evidence remains `partial` or `unknown` because ordinary physical input,
options and platform qualification remain bounded.

The `tools` array drives human review: stable `id`, `label`, `category`, `scope`,
`implementation`, `nativeEvidence`, `gap`, exact `apiIds`, `issueKeys`, source
files and receipts. The `commands` array is the complete exact registry list,
including title, description, availability, source location, runtime argument `parameters` and `toolIds`.
Related tool rows may share APIs; count each command ID once. `sketch.tool`,
constraint/dimension kinds and session targets are dispatch options, not extra
registered commands. Shared lifecycle, projection, capture and inspection APIs
are infrastructure rather than independent CAD tools.

Discovery follows `AlloyWindow::registerCommands()` and all eight helper
registrars. Literal registrations were extracted from seven source files. The
Save/Save As, Box/Cylinder, primitive-dialog, Line/Rectangle and unavailable Measure loops were
expanded explicitly. This worker launched no build or editor. The root supplied an accepted-build
`commands.list` export, then closed its editor. Exact ID sets, titles and
availability match all 397 static entries; runtime argument schemas are retained
in each command. This cross-check caught and corrected the dynamically named
Line/Rectangle pair. It verifies registry completeness, not modeling or native
input. The JSON pins the source commit and records the cross-check.

Important gaps visible before any final burndown:

- Sketch geometry deletion is not established by `sketch.deleteSelection`, which
  removes annotations and a bounded fit-spline point. Additional drawing modes,
  sketch patterns/transforms and full solver diagnostics remain gaps.
- Solid tools remain specialized: countersink-only Hole, New Body-only bounded
  Revolve/Sweep, circular-section solid Loft, equal-distance Chamfer, inward
  Shell, Origin-Z full body pattern and selected feature Mirror groups. Missing
  options and general tools have separate rows.
- Surface Revolve/Sweep/Patch/Offset/Ruled/Stitch/Extend/Untrim and continuity
  inspection remain missing. Existing Loft/Trim/Boundary Fill receipts are
  specific geometry recipes, not unrestricted surface coverage.
- Measure is unavailable. IGES, mesh import, OBJ output, 3MF, DXF/SVG and general
  in-app print audit/refinement remain missing or incomplete. Validated STL
  fixtures do not establish general consumer compatibility or physical fit.
- Suppression, rollback, reference repair, full expression coverage, everyday
  native input, visual consistency, interaction performance and all-platform
  packages remain release work. Production rendering is GPU OpenGL; Metal/Vulkan
  migration remains open under the accepted renderer direction.

Issue keys identify existing scope owners. The local issue mirror is dated;
closed broad parents and accepted ingredients do not prove their whole feature
family complete. Final issue status and bounded child creation belong to the
root's live backlog reconciliation. Missing rows without a narrow child must
receive one before implementation, rather than disappearing behind an epic.

Prior tutorial receipts are retained as evidence of their exact fixtures. The
current user rejects the complete iron outcome: TUT-018 must be reopened and its
visual/shape mismatch inspected. This inventory does not count the prior iron
completion statement as v1 acceptance. Enclosure clearance/physical fit and
separate Browser/native input gates also remain explicit.

For each proposed-v1 row, final acceptance needs a compact tool contract and a
representative from-blank create/cancel/edit/upstream-change/undo/reopen/export
workflow. Keep native input, authored data, exact geometry, mesh readback,
framebuffer quality and packaged runtime evidence separate. A final model or
movie alone cannot discharge all of them. Reuse existing bounded receipts and
add only the missing evidence.
