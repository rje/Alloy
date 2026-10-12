# First profile extrusion and ordered recompute

Research date: 2026-10-11. Proposed implementation contract for HIS-001 and
SOL-001; this document is not implementation or runtime acceptance evidence.
SKT-015 remains the complete sketch → extrusion → upstream edit checkpoint.

## Evidence and limits

Original URLs, sampled images and their hashes are recorded outside the repository
in `../Alloy-reference-records/extrude-first-slice-sources.json`. Stable IDs below
resolve there; the two original tutorial records resolve in
`../Alloy-reference-records/tutorial-gate-sources.json`. No reference application
was operated. Article/transcript evidence is not a claim to have played a video.

| Source ID | Evidence actually reviewed | Bounded finding |
|---|---|---|
| `extrude` | Official command reference | A sole visible profile is automatically selected. Otherwise select coplanar profiles/faces; selecting a sketch through Browser/Timeline selects its geometry. Profile Plane, One Side, Distance and New Body are documented choices. |
| `extrude-solid-howto` | Official step-by-step instructions | Open Extrude, select geometry, set options and click OK. Flip direction and the broader modes are separately documented. |
| `extrude-public-animation` | Sampled official animation frames, 0.00–9.86 seconds | Selected base is blue; preview is a shaded neutral solid with a blue direction arrow and numeric distance. Final solid has no active handles. Taper appears later in the animation but is outside this slice. |
| `tutorial-toy-block-2026` | Kevin Kennedy / Product Design Online original article and author transcript, first extrusion and later visibility passages | Finish Sketch → E → enter distance → inspect from Home/oblique view → OK. A body and timeline feature result. The author later explains that a sketch is automatically hidden after use by a modeling feature. |
| `tutorial-library-die` | Estes Valley Library original article, “Entering the third dimension” | Select square, extrude using handle or number, accept with Enter/OK; oblique view makes the new depth evident. Historical layout and learner-selected dimensions limit exact comparison. |
| `timeline` | Official timeline instructions | Double-click or Edit Feature revisits the existing feature. Broader reorder/suppression behavior is separate work. |

The animation is a cropped curved-sector example, not a whole-window screenshot
or evidence of a supported curved profile in Alloy. Exact build, theme, platform,
font, initial distance default and invalid-input behavior are not established by
these sources. The public 2026 tutorial is a dated edition, not a precise build ID.
Alloy's current adjacent Qt dock is an explicit native UI decision; the animation
crop does not prove its position or spacing. Do not claim a pixel match.

## Bounded user workflow

1. Finish an origin-plane sketch. Its accepted closed region must remain visible
   and selectable outside sketch-edit mode; selection identifies a real evaluated
   profile, not only its wire or a screen-space polygon. E and the toolbar open
   the same Extrude session. Retain an explicitly selected eligible profile;
   otherwise auto-select only the sole visible eligible region. With ambiguous
   input, ask for selection rather than choosing by array order. A sketch
   selection may resolve to its sole eligible region in this slice.
2. Show the selected profile and its parent sketch in the adjacent CommandDock.
   Expose Distance with the document's displayed units and accept explicit unit
   input through the same conversion path as automation. Clearly show the
   supported settings: Profile Plane, One Side, Distance, New Body. Do not expose
   nonfunctional Join/Cut, taper or extent choices. Existing E/Enter/Escape paths
   must respect numeric-editor focus and the shared session's commit/cancel.
3. Preview a real OCCT solid after accepted input. Keep OK unavailable while the
   target is absent, invalid or the distance is invalid. Failed input must clear
   or visibly invalidate the preview; it cannot leave an old shape presented as
   the new result. Previewing and camera changes do not create undo entries.
4. Commit one authored extrusion feature and one body in one undo transaction.
   Automatically hide the consumed sketch as presentation state, while retaining
   a Browser show action and access to its history edit. Cancel restores the
   pre-command presentation and authored state. Auto-hide must not mean deletion
   or suppression, and must not prevent a later intentional source selection.
5. Edit Feature reuses the feature ID and shows its input/profile and distance.
   Hide the replaced solid while previewing its replacement, then restore on
   cancel. Editing the source sketch should make its region usable without a
   dependent solid obscuring it. Temporarily hiding downstream presentations is
   an Alloy design choice, not a researched claim of complete timeline rollback.
   Restore visibility on finish/cancel; never persist a temporary hide as authored
   suppression. Account for all actual downstream dependents, not only the first.

The accepted Alloy policy is a signed distance with magnitude 0.001–100000 mm,
along the stored sketch-frame normal; zero is invalid. Negative distance reverses
the direction. UI, API and saved parameters must share this policy. Provide a
shared Flip control and a direction cue where practical; the reference documents
Flip, so any omitted interaction remains a named compatibility gap. Neither the
initial distance nor an automatic camera jump should be called a researched
default. Use a useful positive seed as an Alloy choice and preserve the user's
camera; Home includes the preview for a clear oblique inspection path.

## Preview and presentation requirements

Use the actual solid with readable shaded faces and crisp edges, a distinct
selected base region, and an unambiguous normal/distance cue. Keep the dock beside
the canvas, with distance, OK and Cancel readable without covering the model.
An editable direction handle is a useful follow-up if the first implementation
uses a noninteractive cue; label that difference explicitly. Avoid treating a flat
top-view screenshot as proof of solid depth. Inspect at an oblique angle and verify
volume independently. Preserve the settled camera during field edits, selection,
preview replacement and redraw; do not reintroduce intermediate camera flashes.

Profile fill and its pick region must use the same evaluated face. Hidden sketches
must not leave selectable ghost fills. A preview must not be entered into the
committed Browser/body cache. The command's API state should expose profile ID,
source sketch ID, distance in canonical mm, preview validity, replacement body ID
when editing, and any error; framebuffer capture must show the same candidate.

## Authored history and kernel boundary

The current paired authored/evaluated snapshot and single QUndoStack remain the
transaction authority. Replace the current separate primitive/sketch evaluation
passes with **one ordered full evaluation**. Each feature resolves its declared
inputs from already evaluated upstream features; missing, forward, mismatched or
cyclic references fail before publishing the candidate. Full evaluation is adequate
for this graph; an incremental engine or second transaction stack is unnecessary.

Persist the extrusion's stable feature/body identity, source sketch ID, derived
profile ID and canonical distance. Do not persist a copied BRep as the authored
input, reference a profile by list index, or silently bind a missing ID to a nearest
region. The existing simple-loop profile ID derives from sketch and source entity
IDs: dimension edits preserve it; deleting/replacing the boundary can invalidate
it. Report the dependent feature and failed input explicitly. Profile geometry
and body geometry in one successful result must come from the same candidate.

For sketch edits, solve and derive profiles, then evaluate downstream extrusions
before the existing atomic undo push. A rejected edit keeps the entire previous
paired document intact. Undo/redo restores the paired state; save/open validates
and recomputes the chain before replacing the current document. Changes to label
placement alone must preserve solved points and downstream shape measurements.
These rules also cover editing distance and deleting a consumed sketch: reject an
orphaning deletion until an explicit dependent-delete policy exists.

Use pinned OCCT 7.9.3
[`BRepPrimAPI_MakePrism`](https://raw.githubusercontent.com/Open-Cascade-SAS/OCCT/V7_9_3/src/BRepPrimAPI/BRepPrimAPI_MakePrism.hxx)
with the evaluated **face** and a finite vector along its sketch-frame normal.
The documented topology rule is face → solid, whereas wire → shell. `Copy=true`
is a reasonable cache-isolation choice. Check completion, valid closed solid,
finite bounds and volume under the existing kernel policy. Its `FirstShape`,
`LastShape` and `Generated` methods offer source/cap/side provenance for later
reference work; do not invent persistent face names from enumeration indices.
OCCT documentation and header retrieval: 2026-10-11, source `occt-prism-793`.

## Proportional acceptance plan

- Core: a 40 × 25 mm rectangle has area 1000 mm²; extrusion by 5 mm is one valid
  closed solid of volume 5000 mm³. Edit width to 50 and observe area 1250 and
  volume 6250. Preserve feature/profile IDs. One non-XY frame fixture checks that
  the prism follows the actual plane normal, not a hard-coded world Z vector.
- Through the real app: Finish → select region → E → distance 5 → preview → OK;
  edit source width; undo/redo; edit distance and cancel; save/reopen and compare
  authored IDs, dimensions, region area and solid measurements. Include a failed
  upstream boundary/reference edit and zero-distance rejection, preserving the
  accepted document. Reuse accepted primitive/sketch cases once for SKT-015.
- Native input: prove committed-profile selection, E, direct numeric typing and
  OK, timeline edit and cancel; inspect the complete dock and an oblique preview.
  Use automation for covered state/captures and physical input for the native
  focus/selection path. Do not repeat the entire suite merely for another image.
- Evidence: record build, API/transactions, geometry, save/reopen, native input,
  hardware renderer and visual review separately. Capture after Finish, preview,
  committed body and upstream edit; compare public animation conventions only at
  a stated comparable camera/state. Record remaining differences instead of
  claiming full command or platform parity.

Face-attached sketches, curved/holed/multiple profiles, expression parameters,
booleans, symmetric/two-sided/thin/taper/offset/to-object extents, suppression,
reordering and general face naming remain their existing backlog work. This slice
does not close SOL-010, GATE-020, platform packages or the full visual backlog.
