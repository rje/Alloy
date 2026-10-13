# Move actions at ordinary window heights

SOL-023 #184, 2026-10-12. Independent native support walkthrough #171 found
Move/Copy's nine numeric fields pushed OK/Cancel outside a 1136×790 window;
resizing to 1136×950 was a workaround. This is an Alloy runtime layout mismatch,
not a new public-reference operation or redesign.

The narrow candidate follows Combine's existing Qt panel pattern: the original
form and errors live in a resizable vertical scroll area; OK/Cancel stay in the
fixed footer. Fields, numeric gestures, command authority and history are unchanged.
Stable widget IDs identify `move.formScroll`, `move.ok`, `move.cancel`.

`widget_interactions_test moveActionsAtShortHeights` checks 1136×790 and the
1040×640 application minimum, reaches Pivot Z by scrolling, verifies buttons
remain inside the window and fixed while scrolling, exercises Cancel without
authored mutation and commits a Move using the visible OK button. This is
synthesized QWidget evidence, separate from native input and actual captures.
The private Qt 6.11.2 / OCCT 7.9.3 macOS arm64 app and widget test compile.
The targeted widget run passes on the native macOS graphics session: three Qt
passes, zero failures,1.738 s. Its first run found real horizontal clipping
(234 px viewport, field reaching241 px) when the vertical bar appeared. The
scroll now reserves form minimum width plus scrollbar width; viewport250 px
fully contains the fields. Test controls are reacquired after prior dock deferred
deletion, and footer coordinates use the existing global-coordinate conversion.
No production Cancel crash was established. Root/Astra should independently
inspect the panel and click its actions at the originally failing height before
closing the issue.

Independent native review also passes at1136×790: scrolling exposes all pivot
fields, fixed OK/Cancel stay readable, and physical Cancel preserves authored
data. The sole private editor was closed afterward.
