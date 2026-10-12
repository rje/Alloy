# Editable CAD samples

## Primitive dimensions

`gauge-block.alloy` is independently authored Alloy data: a 24 × 32 × 10 mm box
and a separate radius-5, height-10 mm cylinder. It contains editable feature
intent and stable IDs, rather than a stored display mesh.

1. Launch Alloy and choose File → Open → `examples/gauge-block.alloy`.
2. Click **Gauge block** in the Timeline. Change Width in the right-docked panel. Leaving a field updates Preview;
   OK commits and Cancel discards. The title-bar controls float or close the panel.
3. Create another Box or Cylinder from the toolbar. X/Y/Z place its origin in
   world coordinates; this first slice creates a New Body.
4. Use Cmd+Z / Cmd+Shift+Z on macOS to undo/redo. The Edit menu exposes the
   platform shortcuts; Windows/Linux still require runtime qualification.
5. Try Home, Top, Front and Right; Option/Alt + left-drag (or middle drag) to pan, and wheel zoom.
6. Use File → Save As to preserve a copy, then reopen it and edit again.

Clicking a Timeline button edits; its arrow menu deletes the feature. Body
selection uses the actual viewport geometry and highlights the selected body blue.
Click empty canvas to clear selection. Reopening preserves editable
features but begins a new empty session undo journal.

## Origin-plane sketches

`rectangle-sketch.alloy` contains an independently authored 40 × 25 mm XY sketch:
four points and four lines with stable IDs. This earlier sample contains no
constraints or dimensions; opening it preserves that authored intent. Newly
created rectangles receive four automatic horizontal/vertical constraints.

1. Choose File → Open → `examples/rectangle-sketch.alloy`, then click its Timeline
   feature to re-enter the sketch. The right Sketch Palette shows its plane and
   authored entity count.
2. Choose **Rectangle** (R) or **Line** (L), then click two points in the canvas.
   Lines continue from the previous endpoint. With the canvas focused, Escape
   stops the drawing tool and clears the pending point; it leaves the sketch open.
3. For precise coordinates, use Start X/Y and End X/Y in the palette, then choose
   **Add Line** or **Add Rectangle**. These are sketch coordinates in the shown
   units. These coordinate helpers author geometry separately from dimensions.
4. **Finish Sketch** commits the session in one undo entry. **Cancel** or closing
   the palette discards its changes and restores the previous sketch. Try
   undo/redo and Save As, then reopen and re-enter the same feature.
5. To start another sketch, choose **Create Sketch**, then XY, XZ or YZ in the
   origin-plane chooser. The view looks normal to the selected plane.

## Driving and driven dimensions

`dimensioned-rectangle.alloy` contains a 50 × 25 mm XY rectangle with a fixed origin,
four H/V constraints, driving width/height and a driven diagonal. The palette and
automation share its retained point, line, constraint and dimension IDs.

1. Open `examples/dimensioned-rectangle.alloy` and click its Timeline feature.
   Choose the horizontal driving dimension in the palette's dimension list.
   Change its value and click **Apply Dimension Value**. The solved edges and
   driven diagonal update in the disposable draft.
2. Try **Cancel**, then re-enter and make the edit again. **Finish Sketch** creates
   one undo entry; undo/redo restores the authored dimensions and solved geometry.
   Save a copy and reopen it to continue editing.
3. In an active sketch, use Select and click a point or line; Shift-click adds
   geometry to the selection. Orange marks selected geometry, green marks fixed
   points, and black/blue indicates zero/nonzero whole-sketch DOF. The palette
   also lists the points and lines for precise selection.
4. Select one line or two points, choose Linear/Horizontal/Vertical, and use
   **Add Dimension**. **Driving dimension** controls geometry; unchecking it adds
   a driven measurement. D opens the dimension selection tool, whose numeric
   entry is currently in the palette. Driven labels use parentheses.
5. Apply coincidence to two points, H/V to one line, parallel/perpendicular to
   two lines, or fixed to one point using the palette. Conflicting or redundant
   relations are refused with diagnostics while the last usable draft survives.

The app currently supports these line/point relations and dimensions. Canvas
dimension placement, inline rectangle entry, double-click dimension editing,
native point dragging and per-entity constraint badges remain
[tracked interaction gaps](../docs/research/commands/sketch-controls.md).
Face-supported sketches, extrusion, solid face/edge selection, Boolean operations,
import and printing export also remain ahead. Disabled controls identify work
still to come.
