# View navigation

Alloy's navigation controls change the camera without changing sketch or solid
geometry. Visible tools and cube actions use the same camera state as Alloy's
other view controls.

## Pan, zoom and orbit

The following mouse bindings describe the current default.

The navigation bar provides **Pan**, **Zoom** and **Zoom Window**. Select one,
then left-drag in the canvas; **Escape** returns to selection. Pan moves the view,
Zoom changes its scale around the cursor, and Zoom Window fits the rectangle you
draw. A rectangle smaller than 8 logical pixels in either direction is ignored.

For a mouse with a middle button, drag it to pan and hold **Shift** while dragging
to orbit. **Option/Alt + left-drag** also pans. To orbit without a middle button,
select **Free Orbit** and left-drag the canvas; press **Escape** to exit. The
adjacent mode menu offers **Free Orbit** and **Constrained Orbit**. Constrained
Orbit keeps the horizon level against the current view convention's up axis.

To choose an orbit pivot, open the blank-canvas context menu and choose **Set
Orbit Center**, then click a visible point on the model. The green marker shows
the chosen pivot. Choose **Reset Orbit Center** from the blank-canvas context
menu to return it to the visible-scene center.

## Orientation cube and standard views

The cube stays orthographic even when the model uses perspective.
Click a labeled face to look squarely at that side; click an edge or corner for an
angled view. Drag the cube to orbit. The cube's **Home** control restores the
saved Home view. Four adjacent-view and two face-roll controls appear beside the
cube when the camera is on a principal face; these turn to a neighboring face or
rotate the face view.

The navigation bar also has **Top**, **Front** and **Right** buttons. In the
Browser's **Document Settings**, right-click **Up** to select canonical Z-up or
Y-up for this document. In **Named Views**, right-click **Front** or **Top** to
make the current camera view that cube face, or reset the coherent Front/Top
frame. The small **UP Z**, **UP Y**, or **UP CUSTOM** label beside the cube shows
the actual cube-up direction. All six standard views, cube regions and
Constrained Orbit follow this frame; model coordinates, sketch planes and
saved Home/custom camera poses stay in world coordinates. This immediately
applied document setting and whole-frame reset are Alloy policies. The public
preference describes an application-level up choice that requires restart;
its exact face-remapping algorithm is not documented.

## Fit, selection and Home

**Fit** frames visible geometry while preserving the camera direction. The
**F6** shortcut is bound to Fit; native key delivery is not yet qualified.
**Fit Selection** frames one selected visible body, planar face or finished
sketch without rotating the camera. **Look At** aligns the camera to a selected
planar face or sketch plane. Unsupported or ambiguous selections leave the view
unchanged.

**Home** restores the document's saved Home camera. Before a Home view is saved,
it fits visible geometry in Alloy's default isometric view. To save one, open the
Browser's **Named Views > Home** context menu and choose **Set Current View as
Home**. Choose **Reset Home** there to remove the saved pose and restore the
default. Saving or resetting Home is a document metadata change; moving the
camera or restoring Home does not add a modeling-history step.

## Camera projection

Open **Camera** on the navigation bar and choose **Orthographic**,
**Perspective**, or **Perspective with Ortho Faces**. The hybrid option uses
orthographic projection at the six principal face views and perspective at
other orientations. Projection is preserved with the open tab's camera state.

## Saved named views

In the Browser's **Named Views** folder, choose **New Named View** to save the
current camera. Edit the new row's name and press **Return**. Use the row's
**Rename** action to change its name later; click a row to restore its camera,
or delete it from the row's context menu. A saved view includes its camera
orientation, projection and framing. It is stored in the document and survives
save/reopen. Creating, renaming and deleting a view are undoable metadata
changes; restoring a view changes only the live camera.

## Input qualification

The visible tools provide pan, zoom and orbit routes for people without a middle
mouse button. Physical Mac trackpad gestures, other platform input and broader
native-device delivery remain unqualified. Application navigation preferences
are available under **File → Navigation Controls**. The **Shift+Alt** preset
uses Shift+Alt with right-drag to zoom, middle-drag to pan, and left-drag to
orbit. On macOS Alt is Option. This preset still needs a middle button for pan;
use the visible tools if you do not have one. **Reset Navigation Controls**
restores the default bindings. The preference is saved for the application,
independently of documents and their undo history.

These controls have focused Qt/GPU evidence in the [navigation receipts](validation/free-orbit-navigation.md),
[projection receipt](validation/navigation-projection.md),
[Fit and Home receipt](validation/navigation-fit.md),
[saved Home receipt](validation/navigation-home-saved.md),
[custom named views receipt](validation/navigation-named-views.md),
[visible navigation tools receipt](validation/navigation-pan-zoom-window.md),
[selection framing receipt](validation/navigation-selection-look-at.md),
[orientation cube receipt](validation/orientation-cube-regions.md),
[cube drag receipt](validation/orientation-cube-drag.md),
[cube controls receipt](validation/orientation-cube-accessories.md), and
[trackpad event-routing receipt](validation/trackpad-navigation.md).
