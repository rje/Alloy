# Enclosure connector and card openings — accepted (#322)

Status: independently accepted and integrated, 2026-10-13. `tests/enclosure_port_card_workflow_api.py` extends the accepted
mounting prefix from a blank document through the connector, long-slot and
rounded card openings. The public sequence and uncertainty are recorded under
the external `enclosure-port-card-contract.md`. The independently authored
nominal board STEP has SHA-256
`f695639a970642c3154f10758acccdafc9d71a8efdace512b1ce057e5805fe27`.
It is a software-validation fixture, not the tutorial creator's file or a
certified physical fit.

After the retained R5/R2.5 shell and four mounting pegs, the runner creates
Offset Plane 0 on the connector wall, starts its sketch, edits the retained
Width section to adapted Y = 32 mm, projects three actual imported connector
faces and offsets each linked rectangle outward 1 mm. Their overlaps and
contacts form four selectable union cells totaling 973.5 mm²; the Cut selects
all four cells once and terminates To Object at the Case inner wall. The source
calls for three connector selections but this nominal fixture produces one
connected clearance opening. The long-wall Overall Slot retains source width
6.5 mm; its Y endpoints -39 and 10.25 mm and Z 10.6 mm are disclosed fixture
placements. On the opposite short wall, the 16 × 12 Center Rectangle uses a
world-axis-centered local position corresponding to X = 0, Z = 0. That local
center is an Alloy adaptation pending the separately tracked face-sketch
origin compatibility issue. Its Cut ends at the inner wall; three exact current
Cut rim IDs drive one R2 Fillet around the exposed U lip. The Case returns to
100% opacity, and analyses and the construction-plane overlay are hidden.

The full real-app run completed 220 successful shared-control calls and 45
captioned stages. The 1120 × 902, 5 fps recording lasts 236.2 seconds, has
1,181 frames and zero capture failures. Representative captures show linked
purple connector projections, the four-cell Offset, the two wall Cuts, the
centered R2 lip and the reopened Case timeline. This is API-driven footage;
the local recorder's Qt-event metadata is not a claim of physical mouse or
keyboard input. DOC-028 separately verifies native Browser section visibility
during a sketch. Numeric section-dialog nesting remains #334.

The saved document reopens with exact authored state, 13 evaluated bodies and
the same three named analyses. Case-only STEP independently reads as one valid
millimeter solid of 59,639.833990 mm³. The strict STL reader finds one closed,
consistently oriented 8,808-triangle surface with exact 64 × 93 × 30 mm outer
bounds and net volume 59,633.607666 mm³, 0.01044% below the CAD value. The
prefix's +0.25 mm board-placement edit and undo precede these openings; it is
not proof of a full downstream port/card edit. Separate focused ingredient
evidence covers retained plane/project/Offset relations and card width edit
through R2 Fillet. Arbitrary closed-loop union selection remains #330.

External deliverables are under
`../Alloy-reference-records/deliverables/enclosure-port-card-2026-10-13/candidate-01/`:
`enclosure-port-card-creation.mp4`, `enclosure-port-card-receipt.json`,
`enclosure-port-card.alloy`, the Case-only STL/STEP and stage captures. The
earlier `diagnostic-01` stopped at DOC-028's `session_busy` gap, now fixed;
the separate off-center card diagnostic is excluded from accepted evidence.

Astra independently reviewed the source-order runner, final receipt, seven
movie samples and final captures, and strict-read the STL without cavity
allowance. The accepted movie, model and exports are linked as the first
entry in the external video gallery; all 82 local gallery links exist.
The port sketch sits near the palette edge in the movie; final close-ups
are centered. This is a bounded functional checkpoint, not complete visual
or native-input parity.
