# Local components independent review

Reviewed 2026-10-13 on candidate 533a00c in the local-components worktree.
Accept bounded ASM-004/#200. The reviewer did not implement the component code.
The larger ASM-001 assembly family and cumulative die tutorial remain separate.

The actual application runner `tests/local_components_api.py` passed named
component creation, distinct definition/occurrence identities, owned centered
sketch/extrusion/fillet, activation without authored/revision/undo changes,
editing from root while preserving ownership, atomic cross-owner refusal,
undo/redo, per-tab activation and exact authored save/reopen. Reopen activates
root as documented. The existing Homebrew Qt 6.11.2 build ran and exited normally.

Component-scoped binary STL export returned the intended child body only,
excluding 100 mm root stock. Independent strict mesh readback found one closed,
consistently oriented component with 3260 triangles, finite unit normals aligned
with winding, bounds [-10,-10,0] to [10,10,20] mm and positive volume
7927.806670 mm³. Root stock remained in the document and viewport.

The verifier-authored Qt case `localComponentDialogActivationAndExportScope`
passed (3/3 including setup/cleanup, 2.788 seconds). It triggers the actual
New Component action, types the name, clicks the Activate checkbox off, checks
Cancel and OK, and clicks Browser root/child radio indicators. A subsequently
created body belongs to the child and appears under that child's Bodies folder.
Activation leaves authored structure/revision unchanged. The export dialog
opens with `Component: die` selected after clearing body selection; Cancel works.
These are Qt-dispatched events, not physical OS input. The actual export write
is verified through the shared API; native file-chooser Save is not claimed.

Inspected captures show readable Name/Activate/Cancel/OK controls, clear root
and child Browser grouping, a child-scoped timeline and a legible export selector
with explicit millimeters and refinement controls. Public instructions establish
named components and Browser activation, but do not establish the checked default
or reopen state; those remain documented Alloy choices. Historical Make/3D Print
labels differ from local Save As Mesh. No matched visual parity or support for
nested/reused/moved occurrences, external references or joints is claimed.

Evidence: `build-components/evidence/components-independent/` contains
`report.json`, `transcript.json`, `die-component.alloy`, `die-component-mm.stl`,
`components-browser.png`, `qt-input.txt`, `qt-new-component.png`,
`qt-browser.png`, and `qt-export-scope.png`.
