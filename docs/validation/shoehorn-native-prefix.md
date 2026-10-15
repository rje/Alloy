# Native shoehorn creation through Thicken

TUT-031, 2026-10-15. Independently accepted by Astra source and final evidence review.

`tests/shoehorn_native_prefix.cpp` starts with an empty editor and creates the
three-point spine, 130/50/16 mm dimensions, retained endpoint-normal planes,
front arc with 50/10/10 mm dimensions, rear 30 mm line, Centerline surface Loft
in a new component and signed +3 mm Thicken. The full creation sequence is
recorded, including the main shape. Thicken preview/Cancel and subsequent
commit use the shared editor commands.

Input is Qt-delivered keyboard and pointer events, not physical device input.
Camera framing, projection, visibility, inspection, capture and saving use the
local API. Menus are opened by the harness before their visible items receive
clicks; scrolling may reveal controls programmatically. Those conveniences do
not establish exact menu-opening or scrolling parity. Initial window activation
is harness setup. No preloaded or API-authored geometry substitutes for the
creation sequence.

Public-source interpretation and provenance remain in the external shoehorn
contracts. The three-point spline and dimensions are documented; interpolation
and tangent conditions are not fully specified. Explicit anchoring is an Alloy
adaptation. The source sheet remains hidden in its original component and the
new solid belongs to root; final organization follows later in the lesson.

Existing guided-prefix and surface-Thicken receipts cover focused geometry,
source edits, history and reopening. This checkpoint adds native creation and
recording evidence, not a repeated geometry suite. Exact grouped R12/R16 corner
rounding remains blocked in GEO-019. Perimeter rounding, generated supports,
hole, final organization and output remain under TUT-017. This is not a complete
printable shoehorn or a claim of exact visual/input parity.

The `native-08` cumulative Qt test passed in 78.96 seconds. Its 76-second,
1250×902, 15 fps recording contains 1,140 frames and no capture failures;
905 repeated frames preserve elapsed time while synchronous work blocks capture.
This is a compact walkthrough, not a smooth physical-pointer demonstration.
The final evaluated solid is valid, closed and positive (17,519.599120 mm³),
with the original valid sheet retained separately. Native thickness entry,
source-face identity and root/component ownership are asserted explicitly.

Evidence lives outside Git in
`deliverables/shoehorn-native-prefix-2026-10-15/native-08/`: the movie and input
ledger, eight stage captures, final inspection result and editable
`shoehorn-native-thickened.alloy`. Earlier numbered attempts preserve harness
framing, schema-expectation, responsive-menu and sheet-pick corrections; those
attempts are not accepted cumulative recordings.

Astra inspected the saved model/result and actual film samples at 21, 42, 62
and 75 seconds, including numeric entry, Loft creation and the final solid.
Readable dimensions and constraint glyphs are visible in the stage captures;
exact matched-reference styling and physical-input behavior remain unqualified.
The movie and editable model are linked in the external gallery
`demos/2026-10-13/index.html#shoehorn-native-prefix`.
