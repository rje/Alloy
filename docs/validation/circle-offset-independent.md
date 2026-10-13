# Circle Offset independent review

Reviewed 2026-10-13 on c6ee735. Accept bounded #192 circle-only Offset for the
#146 tube sketch prerequisite; the complete toy-block gate remains separate.

The named Qt case `circleOffsetFlipPreviewCancelAccept` passed. It uses an
API-seeded/preselected circle, actual Offset action, Flip button, Cancel/OK
buttons, and edit control. It checks exact cancel, signed -1 offset, diameter
4.3 from 6.3, stable target identity on edit, and no duplicate result circle.
This is widget input evidence, not an uninterrupted OS-operated sketch.

An independent real-app run previewed/accepted -1, edited to -1.5, restored -1,
changed the source driving diameter to 8.3 and verified the linked target radius
3.15, then restored the source diameter. The evaluated sketch had disk and
annulus regions. Extruding the annulus 8.1 produced one valid closed tube with
volume 134.868573 mm³. Saved/reopened authored data matched exactly. Inspected
actual controls are readable and show One Side, signed distance, Flip, OK, and
Cancel Offset. This fixture does not claim the final tube's roof attachment.

Evidence: `build-tutorial-polish/evidence/offset-independent/receipt.json`,
`offset-preview.png`, `annulus-extrusion.png`, and `offset-tube.alloy`.
The combined targeted Qt receipt is
`build-tutorial-polish/evidence/loft-independent/qt-input-final.txt`.
