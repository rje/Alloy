# Independent sketch Copy/Paste candidate

SKT-179/#702, 2026-10-20. The bounded route is in-sketch Copy/Paste of one
isolated XY four-Line closed profile. General Move/Copy > Create Copy remains
explicit Todo SKT-185/#708. Root owns continuation-context/backlog integration
and independent acceptance. Copy does not create a Timeline feature or live
source association.

Public provenance is external in
`../Alloy-reference-records/knowledge/specs/skt179-sketch-copy-2026-10-20.md`
(R1–R4, retrieved 2026-10-20). Public evidence documents selecting sketch
elements, context Copy/Paste, platform Copy/Paste shortcuts and optional
placement. Constraint transfer is contradictory/unknown, and exact preview,
confirmation keys, defaults, clipboard lifetime and styling remain unknown.
The policies below are Alloy engineering decisions. No proprietary application
was operated; exact visual or internal-history parity is not claimed.

## Shared workflow and data

In active Sketch Select, select the four ordinary Lines, then context Copy or
Command+C / Control+C. Context Paste on the canvas or Command+V / Control+V
opens the same Paste dock. The clipboard freezes the selected coordinates and
supported internal relations at Copy time, is application-local/transient, and
requires the same document and XY sketch. Copy itself changes no authored data,
selection or history. Unsupported/external/retained-owned selections refuse
without silently removing relations.

Paste exposes X/Y translation in sketch units. The initial numeric offset is
explicitly zero and overlaps the source; it is not presented as a reference
default. Pointer placement attaches the lower-left source bounding corner and
snaps to 0.1 mm; typed fields use their displayed precision. Context Paste uses
its clicked location initially. Preview curves are highlighted orange as copied
geometry while the existing source remains in place. Click, Enter or OK accepts
one sketch journal step. Escape, Cancel or dock close discards placement and
preserves source/selection/history. Both sketch and generic finish routes refuse
an active Paste, including invalid placement. Session cancellation closes the
dock; API acceptance/cancellation share native terminal cleanup. Canvas Paste
press/release takes priority over ordinary edge/Select Other handling.

`sketch.copy`, `sketch.clipboardInspect`, `sketch.paste`,
`sketch.pastePreview`, `sketch.pastePointer`, `sketch.pasteAccept` and
`sketch.pasteCancel` share the native controller. Preview uses X/Y and optional
mm/in units; Paste assigns one transient copy UUID. Every copied point, Line,
constraint and dimension ID is UUIDv5 from that token and original ID, so
numeric previews keep their identities. Pasted IDs persist as ordinary authored
records through history/save/reopen; the token is not a persistent feature.

The selection guard reuses atomic Move's isolated simple four-Line topology and
rigid-solve checks. Internal entity-based Horizontal, Vertical, Parallel,
Perpendicular and Equal constraints are cloned. Driving horizontal/vertical/
linear dimensions between internal vertices are cloned with fresh IDs and
local dimension names; annotation positions translate with the copy. External
relations, Fix/absolute placement, unsupported constraints, driven dimensions,
expressions and user-parameter bindings refuse. No constraint is silently lost.
Only the new copy records and dimension-name counter are added; all original
points/entities/constraints/dimensions must compare unchanged after solve.
Copied coordinates must match the requested rigid translation within 1e-6 mm.

The evaluated copied face must be one closed profile. OCCT face-distance checks
reject overlap, containment and contact with every existing profile at or below
1e-6 mm; the diagnostic includes both profile IDs. The evaluator must produce
exactly one additional profile. Invalid solve, bounds, identity, profile or
downstream solid evaluation refuses atomically. Coordinates and displacement
are finite and bounded by ±100000 mm. Ordinary copied geometry can subsequently
be selected and edited using its own dimensions or Move/Copy; it has no retained
source owner.

## Evidence and bounded limits

External evidence: `../Alloy-reference-records/validation/skt179-2026-10-20/`.
Build is accepted `build-preview-release`, Release/private Qt 6.11.2 and OCCT
7.9.3, Apple Clang, `-j4`. Core, native, real-application, GPU and independent
STL results are recorded separately below. No CI, tag or push.

- `core-final.log`: Copy, atomic Move and retained Move pass 3/3. Copy checks
  complete cloned record counts and disjoint IDs, exact source preservation,
  deterministic preview, one extra profile, overlap/contact refusal, missing
  token, disconnected four-Line selection, unmappable driven dimension,
  external Coincident refusal, source-width independence, ordinary copy Move,
  exact history and authored reopen.
- `native-final.log`, `native-final/native-report.json`: 3/3 functions pass in
  10.534 s. Real receiving widgets handle four Shift-click edge selections,
  context Copy/Paste, platform Copy/Paste shortcuts, pointer/numeric placement,
  Cancel/Escape, Enter and corner-click confirmation, sketch Undo/Redo, exact
  reopen, two separate Extrudes and STL. API Cancel permits a fresh keyboard
  Paste; session cancellation closes its panel. Initial activation is separate;
  no interstep focus or selection repair occurs. A second click-confirm trial
  explicitly reselects source geometry as a demonstrated user step.
- `move-regression.log`: the existing atomic Move native workflow and shared
  lifecycle regression pass 4/4 after the new pointer and shortcut routing.
- `api/report.json`, `api/transcript.json`: 91 real-entrypoint calls pass. The
  app reopens the exact native model, checks cloned internal references and
  disjoint IDs, changes source width 20→24 while every copy record remains
  unchanged, then moves only the copy by (+7,-2). Source/copy profile and body
  IDs survive both edits, Undo/Redo and exact save/reopen. Separate trials check
  invalid copy selection, both pending finish routes, other-geometry edit
  refusal, Look At preservation, overlapping/touching placements, invalid OK,
  Cancel, session cancellation and external constraint refusal.
- The native source bounds remain (10,8)..(30,20) mm, and the copy is
  (45,8)..(65,20), giving a 15 mm gap and two 240 mm² profiles. Two 8 mm Extrudes
  total 3840 mm³. Independent binary STL readback verifies two consistently
  wound watertight components, 24 triangles, Euler 4, bounds
  (10,8,0)..(65,20,8) mm and signed volume exactly 3840 mm³.
- Ordinary free source size editing follows the existing solver placement
  policy: width 24 yields source bounds approximately (8,8)..(32,20), while the
  copy stays (45,8)..(65,20). Moving the copy by (+7,-2) yields
  (52,6)..(72,18). Independent edited STL readback verifies two watertight
  components, volumes exactly 2304 and 1920 mm³, total 4224 mm³, Euler 4 and
  bounds (8,6,0)..(72,20,8). Saved `.alloy` files and both STLs accompany reports.
- Inspected `native-final/context-paste-preview.png`,
  `keyboard-paste-preview.png`, `two-independent-solids.png` and
  `api/edited-independent-solids.png` at native size. The full orange copy,
  unchanged blue source and both sets of size annotations are readable.
  Capture responses identify QWidget chrome plus real OCCT framebuffer with
  pixel dimensions/hash. GPU evidence identifies Apple M4 / Apple,
  OpenGL 4.1 Metal - 90.5, MSAA4, DPR2; API framebuffer 2376×1216 for a
  1188×608 logical viewport. Exact public-reference visual parity is unknown.

Initial native failures are preserved: a field-presence guard mistook an empty
existing Move interaction for active input; guards now test `active`. A later
corner confirmation reached Select Other before Paste; Paste now consumes its
press first and suppresses the matching release. No harness focus or selection
repair was used to hide either defect.

Wider curves, disconnected/multiple groups, cross-sketch/design/plane paste,
construction/projection geometry, linked copies, rotation/scaling and generic
Move/Copy Create Copy are outside this bounded route. Physical AppKit pointer/
trackpad, DPR1 and packaged Windows/Linux runtime remain unqualified. Native
Qt pointer checks do not establish those release gates. Public screenshot state,
scale and theme are unmatched, so exact styling remains unknown. Independent
review is pending. Private fixture editors close after evidence; user PID72618
is preserved.
