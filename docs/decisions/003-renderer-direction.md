# Decision 003: native graphics direction

Accepted 2026-10-11; timing amended by user decision 2026-10-24.

Native backend selection, migration and backend-specific qualification are
post-v1 work unless a material immediate advantage justifies revisiting that
timing. The existing hardware-backed OCCT OpenGL renderer may qualify for the
first stable release. RHI-001, RHI-002 and GATE-045 remain Todo requirements;
GATE-045 is retained under the full-roadmap GATE-110 and is no longer a
prerequisite of GATE-050.

This defers backend implementation, not viewport behavior or visual quality.
GATE-005 and the existing release gates still require actual packaged hardware
GPU diagnostics, framebuffer capture, resize/display scale, picking, selection,
readable CAD edges and overlays, preview and interactive performance on every
declared platform. Unresolved seam visibility, transparency and native-input
acceptance remain with their existing owners. Migration itself proves no fix.

The destination is Alloy-owned CAD presentation built on an established C++-usable
rendering hardware interface (RHI): **native Metal on macOS and Vulkan on Windows
and Linux**. Select and admit a mature RHI through RHI-001; no library has been
chosen yet. We will not build a bespoke low-level graphics abstraction.

OCCT remains the geometry/interchange kernel. Its AIS/V3d/OpenGL visualization is
the current hardware-backed renderer and remains eligible for v1 qualification.
The current app does not
use Metal or Vulkan. Apple documents OpenGL deprecation and recommends Metal;
see the dated `apple-graphics` entry in the source registry.

Keep authored sketch/features, evaluated BRep, display tessellation/semantic IDs
and device resources distinct. Backend handles never enter native document files,
undo transactions or persistent geometry references. Preserve the current shared
UI/control commands, camera and identity contracts during migration. The initial
renderer boundary is ReferenceView; add only the interfaces required by actual
geometry, picking, overlays and capture.

RHI-002 migrates supported surfaces/edges, selection, grid/origin, disposable
preview, cameras, native resize/display scale and screenshot capture. GATE-045
qualifies the cumulative workflow from packages on hardware for all declared
platforms after migration. These backend tasks remain open beyond v1; required
packaged platform and viewport behavior qualification remains part of v1.
