# Isolated native Metal fixture (RHI-004)

2026-10-13 bounded local investigation candidate. The default Alloy build and
AIS/V3d/OpenGL renderer remain. Qt/QRhi is not adopted as the production RHI.
Source is `research/prototypes/metal-rhi/`; its standalone build avoids solver
configuration and dependency downloads. Only explicit root `ALLOY_BUILD_SPIKES`
also enables the disposable target.

## Pinned foundation

Qt6.11.2 private coherent runtime, OCCT7.9.3 and AppleClang17 C++20. QRhiWidget is
public, but QRhi/QShader require GuiPrivate and remain tied to this exact Qt build.
Existing precompiled Qt example shaders and corresponding GLSL are copied
unchanged from Qtbase commit `ef55f427f2c8b410d34f8a7681020a3000cf6866`.
The upstream root REUSE annotations explicitly cover examples under
LicenseRef-Qt-Commercial OR BSD-3-Clause; this experiment uses BSD-3-Clause and
preserves notices/full terms and SHA-256 hashes in the adjacent shader NOTICE.
No qsb/Shader Tools compiler or new rendering dependency was downloaded or shipped.
Metal variants and bundle validity are checked at initialization.

## Actual run

One final coherent fixture sequence passes and exits0 after an earlier fixture
ordering assertion was corrected. Actual backend is **Metal**, device name
**Apple M4**, device type1 as reported by QRhi, Qt6.11.2/OCCT7.9.3. Vendor/device
numeric IDs are0; a full GPU driver/version identification is not available from
this report. Qt's debug-layer request is enabled in source; the persisted runtime
log has no validation messages. Native Metal API validation activation is not
independently established by the absence of messages.

The actual BReps are valid: box volume5120 mm³ and cylinder volume3078.7608005 mm³.
Faces apply OCCT locations and orientation. Four-sample visible faces, depth-biased
fills, thin CAD edge lines, grid and blue selected-face overlay are present in
native framebuffer captures. ID picking uses a separate single-sample linear
RGBA8 target, constant encoded per-face colors, depth testing and disabled
blending. No integer attachment or edge picking is claimed.

Six asynchronous GPU readbacks match independent OCCT ray intersections:
box top, cylinder top, background0, two-object depth occlusion, picking after idle
resource recreation and picking after pending-result release. For depth, the
front cylinder is first drawn and the rear box last drawn, so painter order
cannot satisfy the observed hit. Readback latency in this small run is
2.407–3.506 ms; this is a fixture receipt, not a benchmark/performance claim.
Framebuffer orientation reports Y-down and NDC Y-up; known world points match
GPU face IDs and captures.

Observed logical→physical sizes at DPR2:

| State | Scene logical pixels | Native framebuffer pixels |
| --- | --- | --- |
| Initial | 876×570 | 1752×1140 |
| Narrow | 616×410 | 1232×820 |
| Wide | 1076×650 | 2152×1300 |

Resize, idle recreation and pending-result recreation advance resource creations
1→2→3→4→5, with four releases before final teardown. The pending-result probe
reports submitted storage and an explicit cancellation, then a fresh GPU pick
passes. Native bytes were already complete at release, so only queued Qt
completion/result lifetime was observed pending. Source drains native work with
QRhi::finish and guards stale queued finalizers; actual GPU-in-flight close and
arbitrary busy resize/reparent/device-loss remain unqualified. The window is
cleanly destroyed and the process exits0; the serialized desktop slot is released.

Evidence in the isolated checkout's `build-metal/evidence/`: `report.json`,
`state.json`, empty `app.log`, and `initial.png`, `selected-box.png`, `narrow.png`,
`wide.png`, `recreated-selected.png`, `after-pending-release.png`. Own inspection
shows readable dark edges, neutral real solids, grid/depth occlusion and blue face
selection. Astra independently inspected source and actual artifacts and reports
no bounded #210 blocker; root owns final acceptance and integration.

## Limits

Cylinder faces and 30-segment sampled edges are visibly coarse; edge/face display
coherence and full CAD visual fidelity are future work. Picking reads the complete
ID texture (about8–11 MB here) for a sample, rather than an optimized region.
Hover throughput, general edge picking, production lighting/materials, cameras,
CAD history references and a production local control endpoint are not delivered.
Physical multi-display/DPR transitions were not exercised; ordinary Retina sizing
is observed. Windows/Linux Vulkan, integer-ID capability qualification, distribution,
RHI admission/selection and renderer migration remain RHI-001/RHI-002 work.

## Independent acceptance

Astra independently accepts the bounded RHI-004/#210 experiment at4baad2c
(integrated as5b29523),2026-10-13. Final source, pinned shader bytes/license
provenance, report and all six framebuffer captures were inspected. No redundant
GUI run was performed. Triangle locations/reversed-face winding, geometric
normals and shared face geometry for color/ID passes are coherent. Actual Metal
on Apple M4 with DPR2 produced readable shaded bodies, edges/grid and blue face
selection. Four-sample color and single-sample linearRGBA8 IDs are separate.
Six readbacks match independent OCCT rays, including background and a front,
first-drawn cylinder occluding the later box. Resize pixel sizes and post-resource
recreation selection agree with the recorded state.

The cleanup correction disables callbacks, drains readback storage before release,
and invalidates queued finalizers by generation. The observed cancellation had
native completion already done and Qt finalization pending; fresh picking then
succeeded. It does not prove teardown during in-flight GPU work. Exit0 qualifies
the recorded clean close only.

The cylinder boundary is visibly coarse and shaded faces retain tessellation
faceting. Full-texture readbacks transfer roughly8–11MB each; six observed
2.407–3.506ms latencies are not a benchmark or interaction-performance guarantee.
Numeric vendor/device IDs are0 and no explicit driver-version qualification was
obtained. Face picking is actual; edge IDs are manifest-only. Integer attachments,
physical pointer input, display transitions, Windows/Linux Vulkan, production CAD
quality and adoption remain unqualified. Parent RHI-001/#137 stays Todo.
