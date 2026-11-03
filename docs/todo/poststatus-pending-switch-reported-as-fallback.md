---
id: poststatus-pending-switch-reported-as-fallback
title: PostProcess.getStatus reports a switch not yet applied as a FALLBACK
status: open
priority: high
scope: Graphics (PostProcessStack console)
opened: 2026-11-03
tags: [console, diagnostics, post-processing]
---

# PostProcess.getStatus reports a switch not yet applied as a FALLBACK

## Why

A diagnostic that lies is a defect. `PostProcess.getStatus()` read in the frame right after
`PostProcess.disable(IndirectDiffuse)` prints `IndirectDiffuse: SSGIEffect  <- FALLBACK, you selected off (it cannot
run in this frame: ray tracing unavailable or not ready, or no main directional light)`; about a second later it
reads `off`. Nothing fell back: the selection changed on the console's thread and the render thread has not yet
recorded a frame with it (`selectedOccupant()` against `effectiveOccupant()`,
`src/Graphics/PostProcessStack.console.cpp`, the `selected != effective` branch). Reported by macOS-PA (round 3 of
the GI rounds and again 2026-11-03) and by LinuxB-PA (2026-11-03), where it made a slow TAA run ambiguous: GI still
on, or not (graphics doc 13-12 § 43, the AMD line).

## What remains

- Tell a PENDING switch (the selection changed after the last frame the render thread recorded) from a real
  fallback, and print it as such (e.g. `<- PENDING, applied on the next frame`). Which state the render thread
  publishes for that comparison is the owner's decision (read the stack's recorded-frame state first).
- Same check for `select()` and `setLightingMode()`, which go through the same branch.

## ⚠️ Traps

- The fallback is silent at runtime ON PURPOSE (a trace would be one line per slot per frame): `getStatus` is the
  only place it shows, so its wording must be exact.

## References

- `src/Graphics/PostProcessStack.console.cpp` (`getStatus`); graphics doc 13-12 § 43.
