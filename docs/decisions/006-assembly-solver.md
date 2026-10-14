# 006 — Bounded assembly solver candidate

Date: 2026-10-14. Status: bounded backend independently reviewed; ASM-014 retained
Revolute/Pin workflow accepted. Typed Cylindrical authority core accepted; its
UI/API/native workflow remains a separate gate. Original Alloy code remains MIT.

Use the public FreeCAD OndselSolver snapshot
`4be80eef02a3486cda0d78f3ccbb308d207a9639` (version1.0.1) as a separate
replaceable LGPL-2.1 shared library. The source archive digest, license digest,
retrieval date and public library URL are in
[the source manifest](../../third_party/ondsel/sources.json). CMake downloads the
verified archive into the build directory without vendoring source into Alloy.
[The original ABI](../../src/AssemblySolver.h) uses only POD/C types; third-party
C++ classes/templates remain private to the shared adapter.

The recipe builds the upstream library subdirectory directly as SHARED on all
platforms and uses Windows symbol exports, avoiding upstream's Windows static
default. macOS arm64 build/runtime is locally checked. Windows/Linux compilation,
runtime, deployed replacement and release packaging remain unqualified; the
portable shared recipe is not evidence those platforms work. There is no
macOS-only fatal configuration check or static fallback.

## Authoritative input and transient evaluation

The adapter receives current leaf occurrence world poses, authored joint IDs,
two local resolved origin frames, exactly one root-pinned leaf and an optional
single driver joint. The original solve entrypoint retains Revolute-only angle
semantics. ABI version2 adds typed rotation-radians and slide-millimeters requests:
zero, one or two coordinates on one joint, with slide available only for
Cylindrical. Omitted coordinates are free; explicit zero is a prescription. Rotation is row-major, proper and finite;
translation is canonical millimeters (each component within±1e9). Millimeters
convert to SI once inside the adapter and convert back once on result extraction.
Joint marker Z is the directed axis; angle is marker-J X around that common Z
relative to marker-I X. Values outside[-pi,pi] refuse; turn count is unsupported.
Slide is the absolute signed separation dot(pJ−pI, directed world ZI), not a delta
from the incoming pose. Slide is bounded to±1e9mm. Both requested coordinates
are independently checked in canonical original joint frames.

The document/geometry layer must resolve persistent circular origins before
calling the ABI. It flattens nested leaf occurrence world poses once; a parent
container does not turn independently moving children into a rigid part. Solver
pointers, names and matrices are transient, never authored save state. Each
solve builds a fresh graph under one mutex; inputs stay immutable and outputs
are written only after independent checks pass.

Upstream `ASMTPart.isFixed` constrains absolute zero coordinates, so it does not
preserve arbitrary current Pin pose. The adapter instead uses the library's
mature FixedJoint between an assembly-root marker at the full current pose and
an identity marker on the pinned leaf. No custom constraint equations are added. Each prescribed coordinate uses its
own marker pair carrying exactly the original participant/local frames. This
avoids mutable end-frame interference between simultaneous rotation and
translation motions; original joints and Pin retain separate canonical markers.

The bounded document authority permits at most four retained Revolute/Cylindrical
rows, separate from legacy Slider. Connected joint islands solve independently
through the exactly-one-pin ABI: an authored Pin takes precedence, otherwise the
earliest joint’s Component2 is a temporary gauge at its current rest pose.
Unconnected leaves retain authored poses. Joining islands removes their redundant
temporary gauges; parent containers remain traversal only. The earliest joint
in each island is the sole coordinate driver. Passive coordinate edits and all
passive previews refuse; unchanged coordinate fields may accompany metadata edits.
Both participants retain current circular source bindings with an explicit axial
marker offset. Missing, ambiguous or incompatible geometry refuses.

## Completion and refusal

Success requires every returned full rigid transform to be finite, orthonormal
and determinant+1; every original Revolute origin to coincide and its directed
axes to agree; every original Cylindrical axis to agree with no perpendicular
origin separation; complete Pin pose preservation; and exact signed prescribed
angle and slide attainment. These checks include original relationships the library may
classify/remove as redundant. Translation tolerance is1e-5mm; axis/Pin/drive
angle tolerance1e-6rad; rotation orthonormal/determinant tolerance1e-8.
Composed frames and every scalar metric are checked finite before comparisons.

Results distinguish invalid input, unsupported operation, original-joint/Pin/
coordinate conflict, kernel failure and independently checked success. On failure
output arrays remain unchanged. No general DOF or Jacobian-rank claim is exposed.
Upstream weighted dragging is distinct from exact prescribed motion and is not
an ABI operation in this increment. Preview/edit/Cancel/history/reopen remain
responsibilities of the authoritative document command layer.

## Evidence and limits

The focused `assembly_solver` CTest passes on the actual two shared libraries.
It checks full initial poses, nonidentity full Pin, positive/negative exact
analytic Revolute poses, the independent source-sized four-part three-Revolute/
one-Cylindrical seed, finite proper output rotations, and atomic invalid,
unsupported, excessive-coordinate/turn-count and contradictory-original-joint
refusal. Typed-coordinate coverage includes signed rotation-only, slide-only and
simultaneous Cylindrical requests. The actual nested-leaf fixture core checks
placed BRep displacement, retained source-axis edits through Join, stale-source
atomic refusal, undo/redo/reopen, disconnected islands and passive refusal.
It does not launch a GUI or establish application/artist acceptance.

Detailed public solver admission and earlier headless evidence are external:
`../Alloy-reference-records/knowledge/assembly-solver-admission-2026-10-14/`.
The public adapted piston recipe and source unknowns remain external too.
ASM-014 owns the real retained circular-origin/root-Pin native/API workflow;
ASM-008 owns Cylindrical controls; later motion owns bounded coupled drive.
This backend cannot close those items.

Before distribution retain notices/license, exact corresponding solver source
and changes/build instructions, plus a working replacement/relinking route.
The current development build is not a qualified downloaded package. See the
[dependency policy](../dependencies/README.md).
