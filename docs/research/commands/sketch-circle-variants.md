# Additional Circle command records

Public workflow research retrieved 2026-10-18. Creator/source URLs, video
timestamps, tutorial figures and documented-versus-inferred distinctions are
in `../Alloy-reference-records/knowledge/specs/skt-095-circles-2026-10-18.md`.

| Mode | Documented inputs | Commit and retained behavior | Owner |
| --- | --- | --- | --- |
| 3-Point Circle | Three ordered circumference picks; no radius field documented | Third point completes an analytic circle; point snaps retain logical relations when acquired. [Bounded acceptance](../../validation/sketch-three-point-circle.md) | SKT-095/#500 |
| 2-Tangent Circle | Two straight sketch lines, then radius by pointer or typed value and placement click | [Bounded acceptance](../../validation/sketch-two-tangent-circle.md): placement chooses signed sides; exact reference side rule is unknown | SKT-107/#531 |
| 3-Tangent Circle | Three straight sketch lines; no radius or fourth point | Third line completes circle; [bounded acceptance](../../validation/sketch-three-tangent-circle.md) includes the public rectangle parallel-pair fixture and source edit | SKT-108/#532 |

All three modes repeat; Enter ends the command. Escape after a completed
3-Tangent circle is documented. Exact intermediate Escape behavior, numeric
defaults, current glyph placement, palette styling and general tangent
solution-branch rules are unknown. Arc/circle tangent targets are outside
these documented straight-line creation modes. Keep native input, authored
relations, analytic profile, history, GPU presentation and downstream solids
as distinct acceptance evidence.

The public Circle help also describes switching creation types through Sketch
Palette Feature Options while a Circle tool is active. SKT-109/#533 owns this
family-wide control after the individual modes are accepted; its pending-pick
reset behavior requires an explicit researched or Alloy policy.
