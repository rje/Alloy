# Retained planar regions checkpoint

2026-10-12: the actual rounded-bracket sketch produces three valid, individually
selectable OCCT regions: two complete end disks and the connecting region.
Construction edges remain constraint references but do not bound profiles.

The evaluator splits analytic curves using OCCT, creates bounded planar faces,
and maps each boundary back to authored entities/contact references. Region IDs
use canonical source/contact signatures, excluding coordinates and periodic circle
seams. Existing simple profile IDs remain unchanged. Unsupported ambiguous or
unreferenced contacts reject explicitly; broad intersections, nesting and tiny-gap
behavior remain under SKT-009. This checkpoint does not close that whole issue.

## Evidence

- `build-tutorial/evidence/regions-build.txt`: application build passed.
- `regions-core.txt`: region, legacy-profile and extrusion cases passed; the
  fillet case initially exposed a repeat-solve angle roundoff assertion.
- `regions-angle-core.txt`: region and fillet cases passed after preserving
  authored degree values when both solved radian parameters are exactly unchanged.
  Exact JSON/storage assertions remain; no numeric tolerance hides solver movement.
- `sketch-regions-ledger/report.json`: real-app actual outline, construction,
  pointer midpoint/tangent end circles, paired fillets, three valid regions,
  analytic areas, stable region IDs after radius100→90 and cap100→120, draft
  undo/redo, Finish, all three projected profile picks, and exact save/reopen pass.
  Captures include each selected region and active-sketch annotations.

The initial storage run caught7.1e-15degree authored sweep drift. The repeat-solve
serialization correction addresses that observed cause. Earlier failed runs are
retained with the mismatch artifact; final report identifies the passing run.

Pointer command dispatch shares application logic but is not physical mouse
verification. Native fillet captures separately show actual filled regions;
full native three-region selection and multi-region Press Pull remain ahead.
The next SOL-016 slice unions selected contiguous regions into one prism.
