# Native graphics candidates

RHI-001 research, 2026-10-11. All linked primary sources were retrieved on that
date. This is a shortlist and spike proposal, not dependency admission, renderer
integration or hardware acceptance. [Decision 003](../decisions/003-renderer-direction.md)
requires native Metal on macOS and Vulkan on Windows/Linux; OCCT remains the CAD
kernel.

## Shortlist

| Candidate | Documented fit | Integration cost and remaining questions |
| --- | --- | --- |
| **Qt QRhi / QRhiWidget** | C++ RHI with Metal and Vulkan, already within our Qt foundation. QRhiWidget manages the widget's backing texture and depth buffer; `setApi()` explicitly chooses the backend. | Closest fit to the existing Qt Widgets application. QRhi itself has no source/binary compatibility guarantee across Qt minor versions; retain the exact Qt pin and test upgrades. Widget composition adds an intermediate texture. Native graphics configuration is deliberately limited. |
| **bgfx** | C/C++ renderer abstraction with public Metal and Vulkan backends, supporting macOS, Windows and Linux/X11/Wayland. BSD-2-Clause original library license. | A separate graphics dependency and shader toolchain. Native-window handles are supported, but working Qt/Cocoa and Qt/Wayland embedding, lifecycle and render-thread ownership remain unverified in Alloy. |

Qt's defaults choose Metal on macOS but Direct3D on Windows and OpenGL otherwise;
Alloy must explicitly request Vulkan on Windows/Linux. QRhiWidget is public, while
rendering code using QRhi links `Qt::GuiPrivate`. Reparenting between top-level
windows can replace the underlying QRhi, so release/recreate GPU resources rather
than retaining handles through docking. [Qt 6.11.2 QRhiWidget](https://doc.qt.io/qt-6.11/qrhiwidget.html)

bgfx exposes `PlatformData::nwh` and native-display data, screenshot callbacks and
texture readback. Its windowing documentation warns that custom integrations must
respect platform threading rules. A Qt `winId()` alone does not prove correct Metal
layer ownership or Wayland presentation. [bgfx overview](https://bkaradzic.github.io/bgfx/overview.html),
[bgfx API](https://bkaradzic.github.io/bgfx/bgfx.html)

## CAD presentation and automation

Both candidates supply graphics primitives, rather than CAD selection or geometric
identity. Alloy would upload OCCT-derived triangles and authored sketch/edge display
data, render shaded surfaces plus visible edges, and retain a lookup from transient
draw IDs to authored identities. Tessellation and device resources stay derived;
they must not become serialized document or undo authority.

QRhi provides buffers, graphics pipelines and texture render targets. Its readback
batch supports an explicit completion callback; source textures require transfer
usage, and multisampled textures must resolve before readback. QRhiWidget also
offers `grabFramebuffer()` for an actual rendered image. These provide the needed
mechanisms for Alloy's capture commands. A separate unblended ID pass is a proposed
picking implementation, not a built-in CAD picker. [QRhi](https://doc.qt.io/qt-6.11/qrhi.html),
[readback batch](https://doc.qt.io/qt-6.11/qrhiresourceupdatebatch.html)

bgfx's published picking example uses GPU readback. `readTexture()` returns the
frame at which results become available and requires readback capability; pending
results cannot be reported as completed agent commands. Test real backend support
and keep captured frame/revision identity explicit. [Picking example](https://bkaradzic.github.io/bgfx/examples.html#picking),
[readback API](https://bkaradzic.github.io/bgfx/bgfx.html)

## License boundary

Alloy's original code stays MIT; upstream components retain their licenses. bgfx's
BSD-2-Clause notice must be retained, and its actually selected dependencies and
shader tools need their own audit. [bgfx license](https://github.com/bkaradzic/bgfx/blob/master/LICENSE)

QRhi is part of Qt GUI under Qt's existing licensing model. Qt Shader Tools' runtime
library has an LGPLv3 option; the build-time `qsb` tool is GPLv3. Keep shader baking
at build time and audit exactly which runtime libraries and tools are distributed;
do not label the entire Qt/tool chain MIT. [Qt licensing](https://doc.qt.io/qt-6.11/licensing.html),
[Qt Shader Tools license details](https://doc.qt.io/qt-6.11/qtshadertools-index.html)

Diligent was screened out: its public Core description makes native Metal available
to commercial clients. That does not establish an openly modifiable native Metal
backend under the required distribution model. Its public Vulkan route on macOS
does not meet the selected native-Metal destination. [Diligent Core](https://github.com/DiligentGraphics/DiligentCore)

## Proposed next step

Try **QRhiWidget first**, using our pinned Qt release: it minimizes new dependencies
and native-window glue. This recommendation is an inference from the documented
interfaces and current Alloy architecture. Keep bgfx as the alternative if the
spike exposes a concrete Qt limitation; do not implement two production backends.

The bounded spike should draw one OCCT-tessellated box and cylinder, contrasting
selection, and one sketch rectangle in a real Qt docked workspace. Prove explicit
Metal on macOS and Vulkan on Windows/Linux, actual hardware identity, fractional
logical-pixel picking, one completed framebuffer capture and resize/display-scale
behavior. Exercise close/reopen and resource recreation after reparenting once.
Retain native diagnostics and screenshots; a triangle or successful compilation
alone cannot close RHI-001. Then record the selected version/license closure and
implement the migration through RHI-002. The first sketch workflow continues on the
existing accelerated viewport while this work proceeds.
