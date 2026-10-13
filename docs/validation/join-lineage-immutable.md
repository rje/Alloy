# Join lineage input immutability

SOL-041/#267 fixes one evaluation-order defect in `JoinResults::covers`.
The two-shape OCCT Common constructor executes the operation immediately, before
subsequent NonDestructive settings can apply. The probe now default-constructs
Common, assigns Arguments/Tools, sets NonDestructive and serial execution, then
calls Build. Existing coverage-volume checks, Join integration, Shell behavior,
result ownership and UI are unchanged.

`join_lineage_immutable_test` constructs three real circular-prism participants
and a connecting rectangular profile. It evaluates the actual multi-party Join
twice, checks one valid closed positive-volume result retaining the first body's
producer/UUID, all three participant ownership records and exact repeated named
face/edge lineage and volume. Serialized participant BReps, the source profile
and authored document remain unchanged after each evaluation. Existing
`successive_join_test` also passes, including disconnected intermediate ownership,
later merging, retained cap/edge lineage, edits/history and save/reopen.

The first raw comparison exposed only the top solid's OCCT Free flag changing
from `1100000` to `0100000`; every geometric, pcurve, tolerance and topology line
was identical. OCCT container insertion clears that ownership flag independently
of the non-destructive geometry contract. The fixture explicitly marks retained
participants non-free before taking its serialized baseline. This qualification
is limited to the container flag: no geometry, tolerances or topology are
normalized, and production shapes are not modified to make the test pass. The
original raw before/after BReps and failed log are preserved externally.

Final targeted core 2/2 passes. Evidence lives outside the repository under
`validation/join-lineage-immutable/`: `core-attempt-01.log`, raw
`participant-before.brep`/`participant-after.brep`, `core-final.log` and build/config
logs. No private editor was launched; UI source is unchanged, so no native/GPU
replay is claimed or required. This bounded fix does not establish cumulative
ornament Intersect/Join or resolve the independent Shell input validity risk.
Independent review and main integration are pending at this implementation receipt.
