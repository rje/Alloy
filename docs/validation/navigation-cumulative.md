# Cumulative visible navigation workflow

NAV-005/#411, 2026-10-19. The three accepted control slices
[Fit/Home](navigation-fit.md), [Pan/Zoom/Zoom Window](navigation-pan-zoom-window.md)
and [selected framing/Look At](navigation-selection-look-at.md) now pass one
110-call real-Alloy cumulative API workflow. The dated public source contract
is outside Git at
`../Alloy-reference-records/knowledge/specs/viewport-navigation-compatibility-2026-10-14.md`.
The final run, saved model, nine actual GPU captures and exact command transcript
are external under
`../Alloy-reference-records/validation/nav005-cumulative-2026-10-19/run02/`;
`findings.md` in the parent directory describes the initial failure and fix.

Fit preserves a custom viewing direction, up vector and orbit pivot while
resetting pan and zoom; Home restores isometric. Explicit Pan, cursor Zoom and
Zoom Window modify the expected camera channels, and a too-small rectangle
refuses without changing the camera. Fit Selection frames an asymmetric body,
then Look At aligns to its planar top. An active small sketch retains its draft
and command through Look At/Fit; the finished 10 × 6 mm sketch frames its
visible strokes. A distant visible body expands Fit scale from 78.2 to 17017;
hiding it restores the 78.2 scale and ordinary center. A copied assembly child
at a +90° Z placement and translation frames at its **world** center
(106.5, 53, 23.5) mm; its selected top face aligns at z=30 mm. Body-only and
cylindrical-face Look At, an ambiguous unqualified body instance and a tiny
Zoom Window return structured errors.

The first run exposed a placed-face bug: Look At uniquely found the displayed
world face, then passed its occurrence-qualified four-field identity to the
definition-only sketch-support resolver, which requires three fields. The
handler now checks planarity on that **same uniquely matched displayed BRep**
and uses its world frame. The accepted rerun proves the placed planar result
and unchanged cylindrical-face refusal. Every navigation stage compares
authored data, revision, undo and evaluated geometry before and after;
save/reopen preserves the authored model. The inspected captures report an
Apple M4 GPU-backed OCCT OpenGL framebuffer at 2× display scale. The known
unused planar canvas band remains REL-013 work.

This closes the bounded **shared-command and visible-control integration**
scope. Prior Qt actual-hit tests qualify the individual controls, but this
cumulative run uses the local control API. Physical mouse/trackpad, F6 and
whole-workflow native inputs remain explicitly in
[NAV-008/#414](https://github.com/rje/Alloy/issues/414), with constrained
orbit/pivot input in [NAV-009/#428](https://github.com/rje/Alloy/issues/428).
Packaged Mac and Windows/Linux runtime/GPU qualification remain
[GATE-005/#134](https://github.com/rje/Alloy/issues/134) and the release gates.
No matched public screenshot parity or physical-device behavior is claimed.
