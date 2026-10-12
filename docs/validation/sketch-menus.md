# Contextual sketch menus

Verified 2026-10-12 at `5c58d36`, with independent Astra review. Public command
location evidence is external `knowledge/specs/tutorial-command-locations.json`.

The active sketch now groups existing commands under Create, Modify and
Constraints. Solid Create and Modify also expose their implemented commands.
Actions use the same authoritative handlers as keyboard shortcuts and automation.

The focused Qt `sketchGroupedMenuCommands` check passed. A separate native run
opened Create Sketch, then nested Circle > Center Diameter Circle, Fillet,
Parallel and Normal/Construction through their menus. C activated Circle after
closing a menu without an extra viewport click. A circle entered at diameter
50 and extruded 10 produced one valid closed solid of volume `6250*pi` mm³.

Evidence is local, outside tracked source, under
`build-tutorial-polish/evidence/`: `native-menu-workflow.json`,
`menu-circle-tool.json`, `menu-fillet-tool.json`, `menu-parallel-tool.json`,
`menu-construction-native.json`, `menu-strip-native.png` and
`native-menu-solid.png`. Actual toolbar and solid captures were readable.

Native popup navigation and resulting command state are verified. The capture
tool excludes separate popup windows, so popup pixel appearance remains
unqualified. Menu ordering and some submenu placement remain explicit inference;
this does not establish complete command or marking-menu compatibility.
