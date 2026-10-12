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
   a driven measurement. D opens the dimension tool: click an edge (or first
   select two points), move the pointer, click to place, then type a value and
   press Enter. Double-click a driving label to edit; drag labels to move them
   without changing geometry. Escape abandons pending placement/typing. Driven
   labels use parentheses and are read-only. The editor accepts numbers with
   optional mm/in suffixes; expressions are not supported yet.
5. Apply coincidence to two points, H/V to one line, parallel/perpendicular to
   two lines, or fixed to one point using the palette. Conflicting or redundant
   relations are refused with diagnostics while the last usable draft survives.

6. Click inside the shaded closed rectangle to select its actual evaluated
   profile. Open or self-crossed line loops report diagnostics instead of a region.
   The accepted region remains filled and selectable after Finish.

The app currently supports these line/point relations and dimensions. Inline
rectangle entry, native point dragging, snapping and per-entity constraint badges remain
[tracked interaction gaps](../docs/research/commands/sketch-controls.md).
Face-supported sketches, solid face/edge selection, Boolean operations,
import and printing export also remain ahead. Disabled controls identify work
still to come.

## Sketch to editable solid

[sketch-extrusion.alloy](sketch-extrusion.alloy) retains a constrained rectangle,
its accepted profile and a dependent 5 mm extrusion. The stored shape remains
editable sketch/feature intent, rather than a copied prism or display mesh.

1. Open `examples/sketch-extrusion.alloy` and choose Home to see the solid's depth.
   Its consumed source sketch is hidden; the Browser checkbox shows it again.
2. Click the sketch's Timeline feature. Dependent solid presentations temporarily
   hide while the source region and dimensions are available. Double-click the
   width label, type 40 or 50, press Enter, then **Finish Sketch**. At 25 mm height
   and 5 mm extrusion distance, width 40 produces 5000 mm³ and width 50 produces
   6250 mm³. The extrusion keeps its source/profile/body identities.
3. Undo/redo restores the sketch and solid together. Re-enter the sketch and try
   Cancel; it restores the previously committed solid and source visibility.
4. Click the extrusion's Timeline feature to reopen the right **Extrude** dock.
   Change Distance for a real live preview; negative values reverse the plane
   normal. Home fits the candidate obliquely. OK/Enter commits; Cancel/Escape or
   panel close restores the original body without a history entry.
5. To create another solid, Finish a closed origin-plane sketch, select its filled
   interior and press **E** or choose **Extrude**. A sole visible accepted region
   auto-selects. Enter a nonzero distance; the dock supports **Profile Plane** /
   **One Side** / **Distance** / **New Body**. OK creates one feature and hides the
   consumed sketch in the same undoable edit. With no eligible region selected,
   the panel asks for selection and disables OK.
6. Save a copy and reopen it. Authored parameters and IDs survive, and the whole
   sketch/profile/solid chain rebuilds; the reopened undo journal starts empty.

Distance magnitude is 0.001–100000 mm. The extrusion field uses current display
units, while the API supports explicit mm/in conversion. Typed unit suffixes in
this field, a Flip button, an interactive direction handle, expressions, curved/
holed/multiple profiles, Boolean operations and other extent modes remain ahead.
See the [extrusion evidence](../docs/validation/sketch-extrusion.md) for the tested
geometry, native input and visual boundaries. Full workflow/visual parity and
Windows/Linux runtime remain unqualified.
