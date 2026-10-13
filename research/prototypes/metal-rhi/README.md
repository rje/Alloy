# Isolated Metal CAD-mesh investigation

RHI-004/#210, Qt6.11.2 + OCCT7.9.3, C++20. This disposable executable is an
investigation, not RHI admission, renderer selection or production migration.
The default Alloy app and its authoritative OpenGL viewport are unchanged.
It can be built independently, without configuring Alloy's solver targets:

```sh
cmake -S research/prototypes/metal-rhi -B build-metal -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/opt/homebrew \
  -DQt6_DIR=/absolute/private-qt-install/lib/cmake/Qt6
cmake --build build-metal
```

The same target is available only when the root `ALLOY_BUILD_SPIKES=ON` option is
explicitly enabled. GuiPrivate ties it to the exact Qt build. Use the matching
private Cocoa plugin in its own bundle; never replace a shared plugin or change
the user's application. On this checkout the private runtime is the existing
`../Alloy-dependencies/qt-private-runtime/install`. No shader tool/download is
needed. The [shader notice](shaders/NOTICE.md) records upstream commit, hashes,
corresponding source and BSD-3-Clause license; the exact bundles include Metal
variants and initialization verifies them.

Run only after obtaining the serialized private-editor slot:

```sh
python3 research/prototypes/metal-rhi/run.py \
  --app build-metal/metal_rhi_spike.app/Contents/MacOS/metal_rhi_spike \
  --output build-metal/evidence
```

`--self-test --output <directory>` drives the same real scene's inspect, capture,
pick and resource functions, emits `report.json`/`state.json`, captures native
framebuffers and exits. An interactive launch offers Inspect, Capture, Recreate
GPU resources and click-to-select through the same asynchronous GPU ID pass.
There is no authoring model, save/history API or replacement production command
service in this fixed read-only sample.

Two actual OCCT solids are tessellated with applied face locations and reversed
face winding. Constant geometric-normal-derived face colors use the pinned Qt
color shader. CAD edges use OCCT curve samples; a ground grid and blue selected
face overlay share the visible depth target. Four-sample color rendering is
separate from the linear RGBA8 single-sample ID target, whose blending remains
disabled. Transient GPU buffers never own semantic identity. Fixture IDs are
object/face/edge names tied to this unchanged CPU fixture; they are not general
persistent references through future CAD edits. Only face IDs are GPU-picked;
edge IDs are an inspectable manifest, not qualified edge picking.

The fixture draws the front cylinder before the rear box. An independently
computed OCCT ray hitting both must return the cylinder's face from the GPU;
last-drawn painter order would return the box instead. Known box/cylinder cap
points and a background miss separately check semantic/readback orientation.

Resize recreates owned device resources at actual pixel size. A bounded lifetime
probe releases resources while submitted readback result/completion storage is
pending; callbacks are disabled, documented `QRhi::finish` drains native work,
and generation checks prevent stale queued completions. Cancellation is explicit,
then fresh picking must work. The observed Metal probe already had native bytes
ready before release, so it proves pending Qt completion/storage handling rather
than a native GPU-in-flight close. Arbitrary busy resize/reparent/device-loss
behavior is not qualified; ordinary Recreate waits for idle.

See [the local evidence receipt](../../../docs/validation/metal-rhi-spike.md).
The coarse cylinder/edge sampling and full-ID-texture readback intentionally
remain spike limits; do not treat this as production CAD presentation or a
performance ranking. Windows/Linux Vulkan, integer ID formats, physical display
scale transitions, broader input/cameras and package admission remain outside it.
