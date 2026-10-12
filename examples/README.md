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
four points and four lines with stable IDs. It is editable sketch geometry, rather
than a solid or a constrained/dimensioned profile.

1. Choose File → Open → `examples/rectangle-sketch.alloy`, then click its Timeline
   feature to re-enter the sketch. The right Sketch Palette shows its plane and
   authored entity count.
2. Choose **Rectangle** (R) or **Line** (L), then click two points in the canvas.
   Lines continue from the previous endpoint. With the canvas focused, Escape
   stops the drawing tool and clears the pending point; it leaves the sketch open.
3. For precise coordinates, use Start X/Y and End X/Y in the palette, then choose
   **Add Line** or **Add Rectangle**. These are sketch coordinates in the shown
   units. They author geometry; driving dimensions are not implemented yet.
4. **Finish Sketch** commits the session in one undo entry. **Cancel** or closing
   the palette discards its changes and restores the previous sketch. Try
   undo/redo and Save As, then reopen and re-enter the same feature.
5. To start another sketch, choose **Create Sketch**, then XY, XZ or YZ in the
   origin-plane chooser. The view looks normal to the selected plane.

Constraints, dimensions, face-supported sketches, extrusion, face/edge selection,
Boolean operations, import and printing export remain ahead. Disabled controls
identify work still to come; the selected solver's native fixture does not enable
application constraints yet.
