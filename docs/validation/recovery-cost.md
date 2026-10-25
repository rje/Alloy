# Single-serialization recovery snapshots

REL-057 / #738, 2026-10-24. Recovery now serializes its full unsigned envelope
once, hashes those exact bytes, and appends the fixed `sha256` member containing
only hexadecimal characters. It retains the existing `alloy-recovery/1` format,
reader, identity/checksum checks and complete authored content. JSON member order
is immaterial to the existing reader, which removes the checksum and uses Qt's
canonical serialization to verify it. A missing final object brace fails closed.
Ordinary readable `.alloy` save encoding is unchanged.

The measured duplicate was serialization, not indentation or atomic disk I/O.
Recovery was already Compact JSON. The exact capacity Box recovery record is
**11742240 bytes**, before and after this change. The approximately 89.34 MB
figure in earlier attribution describes the ordinary indented native file, not
recovery bytes. No compression, external mesh reference, blob archive, global
cache, validation flag or change to mesh precision is introduced.

## Matched measurements

Same macOS Apple M4 host and Release `build-preview-release`, Qt 6.11.2 private
SDK and OCCT 7.9.3; three real-application API samples each, using the accepted
200000-facet / 600000-vertex STL. Each Box is 4 × 5 × 6 mm; Undo returns to the
imported authored document between samples. Both builds include the XCH-020
validated-owned-mesh reuse. Opt-in `ALLOY_TRACE_RECOVERY_COST=1` logs unsigned
encoding, SHA-256, final-member encoding, write/open/permissions and atomic
commit separately.

| Wall time, ms | Before samples | After samples | Median before → after |
| --- | --- | --- | --- |
| Normal Box | 336.9, 203.1, 186.1 | 351.8, 210.2, 289.5 | 203.1 → 289.5 |
| Box after capacity import | 1037.8, 1028.1, 1016.4 | 836.2, 761.4, 742.3 | 1028.1 → 761.4 |
| Summed recovery stages for those Boxes | 717.7, 712.9, 715.2 | 457.3, 421.8, 433.2 | 715.2 → 433.2 |

Total capacity Box median improves **25.9%**, and measured recovery-stage median
improves **39.4%**. The removed second full serialization costs 284.6–294.7 ms
before, versus 0.001–0.003 ms to append the checksum after. First encoding remains
284.2–290.8 ms before / 291.2–305.9 ms after, and hashing 131.0–134.7 ms before /
121.9–139.9 ms after. Write/open/permissions is 3.1–5.3 ms before / 5.4–8.9 ms
after; atomic commit is 1.8–2.1 ms before / 2.9–3.9 ms after. This is a CPU and
allocation-work reduction; snapshot bytes and required atomic publication remain
unchanged. Remaining serialization/hash and refresh costs remain subject to the
wider REL-049 performance gate; this bounded result does not claim optimal or
negligible latency.

Normal Box is noisy and its tiny recovery record saves negligible time; no normal
Box improvement is claimed. Host load differs materially from the earlier
XCH-020 run, so its 561.5 ms capacity median is not a baseline for this change.
The direct stage comparison identifies the work removed despite this variance.

RSS was sampled by `ps` approximately every 25 ms plus operation endpoints.
Baseline RSS is 184.2 MB before / 131.6 MB after; matched benchmark sampled peak
is 616.3 MB / 455.6 MB (decimal MB). These initial conditions differ, OS reclaim
and sampling vary, and sampled RSS is not a hard allocation peak. No causal
memory reduction claim is made. Complete per-operation values are retained.

## Preservation evidence

The focused `recovery_store` CTest passes. It checks canonical checksum equivalence
of appended-member and legacy Qt-serialized records, legacy reading, malformed
and injected-member refusal, unsupported versions, independent per-tab records,
live owner exclusion, failed adoption preserving stale data and durable adoption
before retirement. A real unwritable-directory replacement fails and preserves
the prior valid snapshot. Existing owner-only permissions, symlink refusal,
`QSaveFile` atomic commit without direct-write fallback and lock handling remain
unchanged. Recovery revision advances only after success; the production command
and stale-candidate lifecycle are untouched.

The existing `tests/recovery_api.py` gains an optional `--mesh-fixture` for the
accepted capacity source. Its actual application interruption roundtrip passes:

- Two tabs retain distinct unnamed/named committed documents and stable IDs.
  The owned STL copy is deleted before killing the private writer.
- Recovery evaluates and adopts the full retained mesh without that source.
  Full pre-crash durable authored data exactly equals the recovered readable
  native file, including all coordinates and triangle indices. API metadata and
  the final page (vertex `[498,1599,0]`, triangle `[599997,599998,599999]`) match.
- Live sessions remain hidden; malformed/newer candidates refuse without changing
  the current document. Failed native Save retains dirty state, safe Save As
  preserves the original named file, and retired stale records are removed only
  after durable adoption. The final private editor exits cleanly.

The initial optional-fixture run exposed a harness assumption: `document.inspect`
intentionally removes bulk mesh arrays, so its authored view cannot equal a full
native JSON file. The harness now compares full pre-crash durable authored data
for that check; metadata and paged geometry checks remain independent. No product
change followed that assertion, and the corrected targeted replay passed.

External evidence:
`../Alloy-reference-records/validation/recovery-cost-2026-10-24/`, with
`before/` and `after/` reports/logs, `stage-summary.json`, initial `roundtrip/`
logs and successful `roundtrip-final/{result.json,transcript.json}` plus recovered
native files. The original capacity fixture and XCH-020 visual/lifecycle evidence
are reused; no unchanged native-input or broad geometry corpus was rerun.
The user editor PID 72618 is preserved. No CI or native focus testing occurred.
Independent Sol review covers the bounded implementation and final evidence.
