# As-built Revolute and root Pin

Status: independently accepted and integrated at `96c032c`. Main application
build and focused `assembly_revolute` check pass (1/1, 1.04 seconds). Public behavior and the bounded selection policy are recorded
outside the repository in
`../Alloy-reference-records/knowledge/tutorials/piston-subassemblies-2026-10-14/source-contract.md`
and `asm014-retained-contract.md` beside it.

One as-built Revolute joins two distinct root-level identity leaf occurrences.
The fixed Component 2 supplies a retained, complete circular edge on its own
body; this slice admits a Y-axis circle and records a common as-built frame at
creation. Component 1 starts at its authored rest pose. Angle 0 preserves that
pose; signed prescribed angles from -180° through +180° rotate its occurrence
through the replaceable assembly-solver adapter. The source sketches and solid
definitions stay in rest coordinates. The moving marker remains retained when
the fixed source circle moves after an upstream edit. Changing Position or
participants requires a new joint, avoiding an accidental change of datum.

The root-level fixed component can be Pinned through a real `assemblyPin`
Timeline feature. Its full rigid pose is identity in this bounded slice. Before
Pin, the same fixed component is an explicit temporary solver gauge; inspection
distinguishes that state from an authored Pin. Evaluation before the Pin feature
does not borrow the later constraint. Pin on a Slider, a moving occurrence or a
nested participant is refused. `joint.previewAngle` is transient; `joint.angleSet`
and `joint.pin` are undoable and persist through save/reopen. Slider-only preview
commands reject the Revolute record rather than interpreting degrees as mm.

Focused `assembly_revolute` core passes on the accepted native four-solid
fixture. It checks Angle 0 exact rest BRep, ±15° exact drive and solver residual,
fixed Lower and moved Upper world bodies, Pin prefix/undo/redo/reopen, upstream
Lower thickness edit moving the retained circular origin, and atomic invalid
Position/angle refusal. The 29-call actual-app run at
`../Alloy-reference-records/deliverables/piston-revolute-2026-10-14/api-05/`
passes the shared commands, transient preview/stop, -15° drive, Pin history,
save/reopen and two deliberate command refusals; its saved `.alloy`, ledger and
GPU captures were inspected. The Qt native test at `native-03/` passes 3/3:
the harness triggers the toolbar QAction and assigns component combo indices,
then delivers an actual viewport circle click, types the Angle, uses Cancel,
and commits through OK. The second Position selection is assigned through the
combo. Its readable dock, highlighted circle and rotated preview were inspected.
All private editor processes exited and the GUI lease was returned.

This is one prescribed Revolute, not the coupled 3R+1C piston mechanism.
The nested piston pieces remain stationary while the Upper arm rotates; full
linkage, cylindrical joint, linked copy and motion recording remain separate
ASM-015/ASM-016 and cumulative workflow work. No physical toolbar/dropdown
gesture, general circular axis, changed-datum edit, nonidentity Pin, multi-joint
graph, Windows/Linux runtime or reference visual parity is claimed.
