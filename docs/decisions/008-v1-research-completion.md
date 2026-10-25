# Decision 008: first-v1 research completion

2026-10-25. Research decisions and original fixture probes are complete; bounded
independent acceptance is recorded with the issue closures. This does not qualify
production modeling, native controls, packages, GPU behavior or performance.
Original Alloy code remains MIT; existing foundations retain their own licenses.

## Control contract

Readable original public screenshots establish editable Variable Fillet Start/
End Radius inputs and fixed normalized Position labels0.00/1.00. Current published
endpoint/midPosition API semantics corroborate them. This corrects the earlier
ambiguous editable-position wording without inventing movable endpoints.
Additional radius points remain later work. The public screenshot is from2019;
no current-generation pixel/default claim or proprietary runtime observation is
made. Source V1R-CTL-01/02/03 and the inspected images remain external.

SOL-064 uses the existing OCCT7.9.3 two-endpoint linear law on the isolated
straight edge, normalized against actual kernel bounds. Exact reference-kernel
interpolation is not publicly established; the linear law is an explicit Alloy
choice with declared geometry/contact checks. The selected behavior, defaults,
controls and original fixtures for the other modeling minima are fixed in their
work orders and external V1R-CTL-20261025 record.

## Format foundation and admitted directions

| Format | First-v1 read/write decision | Implementation owner |
| --- | --- | --- |
| STEP | Reuse admitted single-solid OCCT7.9.3 STEP path, explicit units and represented names/placement; wider XDE behavior retained | XCH-025 |
| IGES | Add existing OCCT7.9.3 TKDEIGES; one trimmed planar sheet with rectangular outer boundary/circular hole, mm/in, explicit empty healing flags and strict complete nonempty transfer | XCH-026/027 |
| STL | Reuse accepted strict mesh import and selected-solid/open-sheet export | Existing retained-mesh/export owners |
| OBJ | Reuse accepted strict owned mesh reader and material-free writer; no unnecessary OCCT OBJ modules | Existing strict OBJ/export owners |
| Core3MF | Reuse admitted lib3mf2.5.0 export; import/extensions remain later XCH-005 | XCH-011 and cumulative gates |
| DXF | Owned C++20 strict ASCII AC1015 LINE/ARC/CIRCLE/LWPOLYLINE-bulge adapter; double precision, explicit units/layers and no silent repair | XCH-028/029 |
| SVG | Existing Qt6.11.2 QtCore QXmlStreamReader plus exact bounded M/L/C/Z path/transform adapter; authored Lines and cubic splines, explicit physical sizing | XCH-030/031 |

Source commits, library/build/license snapshots, exact entity graphs, layers/
units/transforms, legal original fixtures and checker hashes are fixed in
V1R-XCH-20261025. The IGES headless probe passed mm/in originals and roundtrips
with healing disabled and an independent finite raw-file analytic checker.
A raw OCCT Done result with zero transferred entities is not import success.

A compatible BSD Dime candidate was built/probed before choosing the DXF adapter.
Its float coordinates, six-digit writer and permissive malformed polyline
truncation require both a numeric fork and the substantive strict per-entity
parser. The bounded owned adapter avoids maintaining both; it is not a claim
that no compatible DXF library exists. MIT ezdxf1.4.3 supplies dev-only independent
fixtures/readback, not a Python runtime in the application. QtSvg is a renderer;
NanoSVG's float/cubic normalization and permissive omissions do not preserve the
required exact source Lines and strict diagnostics. Reuse QtCore XML rather
than adding a second lossy SVG parser.

## Finite mesh auditor

PRT-017 uses installed OCCT BVH pair traversal with original triangle identity
preserved through BVH reordering. Its narrow phase uses orientation-sign overlap
with the pinned public-domain Shewchuk adaptive predicates. The MIT reference
control flow is readable and source-pinned; its fixed-epsilon arithmetic and
floating basis construction are not adopted as robust predicates. Coplanar tests
project original coordinates on the dominant axis. No JavaScript/Three.js or GPL
CGAL runtime dependency is admitted.

The exact C source, public-domain statement, strict floating-point/build rules,
algorithm source pins, vertex identity policy and nine-case corpus are external
V1R-PRT017-20261025. Crossing and coplanar cases are mandatory checked paths;
unsupported numeric classes cannot masquerade as geometric passes. Production
integration and corpus/native/export qualification remain PRT-017/018/003.

## Budgets and research readiness

[Performance targets](../v1/performance-budgets.md) are fixed Alloy engineering
policy before optimization, not reference benchmarks or measured passes.
REL-004/016/049 still measure and qualify them. Actual desktop input failures,
renderer bug diagnosis, missing production parsers and unavailable platform
hardware are implementation/qualification work, not undefined reference behavior.

All109 open first-v1 work orders were checked for material preimplementation
questions. The detailed readiness table and source/visual/probe records remain
outside Git in `../Alloy-reference-records/knowledge/specs/v1-research-completion-2026-10-25/`.
Known source-generation/pixel/default limits remain explicitly labeled; they do
not leave the selected supported behavior undecided or establish exact parity.
Research gates XCH-001 and SOL-099 close only after independent evidence review;
their dependency links remain as accepted decision traceability. Any genuinely
new contradiction discovered during implementation becomes a bounded explicit
issue, rather than silently changing the frozen behavior.
