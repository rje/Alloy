# Initials keyring runner preparation (#233)

Status: prepared, **not executed or accepted**, 2026-10-13. Source ID
`warwick-initials-keyring-20261013`; the independent six-page public lesson
contract, creator, retrieval date, URL, page observations and uncertainties are
stored outside the repository in
`../Alloy-reference-records/knowledge/tutorials/warwick/initials-keyring-independent-contract.md`.
This worktree starts from main `add7b2a` and adds
`tests/keyring_tutorial_api.py`, a proposed blank-to-final shared-command
runner with optional full creation movie. No Alloy editor was launched for this
preparation.

The runner maps source steps 1–21 to an actual two-point Rectangle from the
origin with previewed 60 × 30 entry; four successive side-pair R6 fillets;
2 mm New Body; explicit top-face sketch and retained whole-loop −1.5 mm
Offset; 1.5 mm annular Join; supported-face 2-Point Circle with diameter
endpoint and direction followed by typed Ø4; −10 mm Cut; upper circular hole
edge R1 solid Fillet; modeless Text preview and both in-plane handles; 1.5 mm
multi-region text Join; then Save. A complete movie would show command
captions, previews and representative actual OpenGL captures. The runner
checks authored references, analytic source/rim/cut areas and volumes, closed
one-solid evaluations, glyph holes, same-slot text edit and undo/redo/reopen.
It uses independent strict STL readback for one oriented closed component.

Fixture choices are explicit. XY spans x0…60 and y−30…0. The 60/30 post-fillet
annotations are measured overall reference spans after their original driving
relations retire. The lesson leaves a face-sketch creation transition implicit;
the runner chooses the actual z2 floor cap, then demands that same unique
retained cap through rim Join, Cut and hole Fillet before Text. Its Ø4 written
value takes precedence over a later apparent Ø5 screenshot. The first diameter
endpoint (5,−15), direction +X and resulting center (7,−15) are fixture
choices, not measured public dimensions. Personalized DSB uses the licensed
pinned DejaVu Sans 2.37 Bold asset at 14 mm, moves its baseline from (5,−20)
to approximately (50,−5) by the real handle command, then rotates about 180°
with the other handle. The source offers font, size and rotation examples;
these are declared choices, not claims of a default or exact font match.
Export and the DS8 same-slot edit are Alloy-owned checks after the source Save.
The complete recording would be shared API operation of real commands;
accepted focused native ingredient tests remain separate from physical OS input.

**Integration blocker:** current Extrude Cut creates a bore but omits the Cut
tool's rim-edge provenance from `solidEdges`. The runner requires one upper
Cut-hole `baseRim` identity at z2 and then selects it for solid R1 Fillet.
It will fail at that assertion if the edge is unavailable. Root opened bounded
SOL-034/#236 for the required retained upper circular rim and broad-floor
support lineage; this runner deliberately does not fillet another edge or move
Text before the source-prescribed Fillet. Rounded-loop Offset #229 and native
2-Point Circle #230 must also finish integration before a cumulative run.

Preparation checks: Python syntax compilation, `git diff --check` and the
repository forbidden-byte scan passed. They establish script readiness only.
The next real-app pass requires the integrated prerequisites, one private GUI
slot, full movie/model/STL artifacts outside Git, source-step and geometry
ledger, capture inspection, then bounded independent compatibility review.
