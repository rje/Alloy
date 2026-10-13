# Native Metal/Vulkan foundation shortlist

RHI-003/#208, independently reviewed 2026-10-13. This is accepted research,
not dependency admission, a renderer selection, or runtime evidence. RHI-001/#137
remains Todo for the actual spike. The current AIS/V3d/OpenGL viewport remains.
Primary URLs, retrieval provenance and detailed comparison are external in
`knowledge/sources.json` and
`knowledge/records/rhi-native-metal-vulkan-shortlist.json`.

| Candidate / source ID | Documented fit | Qualification before adoption |
| --- | --- | --- |
| QRhi/QRhiWidget — `rhi-qt-612-20261013` | Native Metal/Vulkan options within QWidget; managed render target, pixel sizing, MSAA and framebuffer capture. | QRhiWidget is public API, but underlying QRhi/QShader classes have no source/binary compatibility guarantee and require GuiPrivate. One top-level widget window uses one RHI backend. |
| SDL_GPU — `rhi-sdl-3416-20261013` | C API callable from C++, Metal/Vulkan backends, native window wrapping, capability queries and transfer/readback resources. | Cocoa NSWindow/NSView wrapping does not prove Qt child embedding, input ownership or lifecycle correctness. Multiple shader formats or shadercross tooling are needed. |
| bgfx — `rhi-bgfx-20261013` | C/C++ interface, Metal/Vulkan, native platform handles, capability queries, render targets and screenshot callbacks. | Qt hosting/frame-loop ownership, CAD edge rendering, format-specific ID passes and readback latency require a spike. Pin bx/bimg and shaderc with the library. |
| Dawn — `rhi-dawn-20261013` | C/C++ WebGPU implementation, Metal and Vulkan, WGSL/Tint shader translation. | macOS13+ Metal and Linux Vulkan are listed supported; Windows Vulkan is explicitly best-effort. Qt surface integration and WebGPU limits require evaluation. |

Qt is the most direct existing-widget integration to investigate first; SDL_GPU
and bgfx offer alternatives if Qt API stability or integration constraints are
unacceptable. Dawn remains credible but its Windows Vulkan support status and
WebGPU abstraction are material tradeoffs for this destination. This order is
an inference about investigation cost, not a chosen dependency or performance
ranking. No library implements CAD constraint semantics or retained identity.

## Version and license boundary

The review checked official releases: SDL `release-3.4.16` at `fa2c02b`;
bgfx.cmake `v1.162.9524-580` at `eb88f6b` (wrapper snapshot with dependency
revisions, not a unified runtime qualification); Dawn `v20261009.190412` at
`c943ea32b614ff53e4fe410fc8f4968b6b8fe46d`. Dawn's release page labels native
binary artifacts best-effort and unsigned; official source is its source-host
repository. These are research anchors, not new project pins or indefinite
latest-version claims. A future build must record full immutable revisions.

Qt research uses6.12 documentation. The current local private Qt package reports
6.11.2; do not imply the newer API reference is the deployed build. Any spike
must verify its exact Qt headers, features and toolchain together.

Primary license texts were independently inspected: SDL uses zlib and bgfx
BSD-2-Clause. Dawn's default is BSD-3-Clause, with Apache-2.0 exceptions and
separate third-party terms. Qt GUI/Widgets licensing and actual deployed
configuration remain part of dependency admission. Qt Shader Tools distinguishes
its runtime library (LGPLv3/GPLv2 or commercial) from the qsb build tool
(GPLv3 or commercial); document build-tool use and shipped components separately.
This comparison does not relicense dependencies under Alloy's MIT license.

## Smallest useful subsequent spike

Render real OCCT box/cylinder triangulations, depth-tested faces, CAD edges and
a small grid/selection overlay in the existing Qt window. Preserve application
face/edge/object IDs independently of transient GPU handles. Exercise hover and
click selection through a separate single-sample ID pass and asynchronous
readback. Integer attachment choice is a hypothesis requiring capability checks;
MSAA color capture does not prove integer picking support. Verify readback
orientation, semantic result and latency, with an explicit tested fallback if
an attachment format is unavailable.

Run the same fixture on actual macOS Metal, Windows Vulkan and Linux Vulkan
hardware. Record device/backend/vendor/version, logical/physical pixel sizes,
resize and display-scale transitions, resource recreation/destruction, native
framebuffer captures and validation output. Inspect thin edges, depth bias,
selection visibility and capture quality; no triangle-only or initialization-only
result establishes CAD viewport readiness. Keep implementation effort, API
support, package build and hardware runtime evidence separate.

The independent review confirmed the primary backend, license and release claims
and requested corrections to the external record: public QRhiWidget versus
limited-compatibility QRhi family; best-effort versus unsupported Windows Vulkan;
and tool/runtime/license distinctions. No application was launched or rebuilt.
