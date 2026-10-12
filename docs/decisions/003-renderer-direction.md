# Decision 003: native graphics direction

Accepted 2026-10-11. Implementation remains future work.

The destination is Alloy-owned CAD presentation built on an established C++-usable
rendering hardware interface (RHI): **native Metal on macOS and Vulkan on Windows
and Linux**. Select and admit a mature RHI through RHI-001; no library has been
chosen yet. We will not build a bespoke low-level graphics abstraction.

OCCT remains the geometry/interchange kernel. Its AIS/V3d/OpenGL visualization is
the current hardware-backed prototype renderer, and continues to serve the first
sketch workflow while the replacement is researched. The current app does not
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
platforms. These tasks remain open. Sketch creation is the next main modeling
milestone; renderer migration and package qualification do not delay its first
origin-plane rectangle/extrusion checkpoint.
