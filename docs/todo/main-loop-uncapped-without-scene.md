---
id: main-loop-uncapped-without-scene
title: Without a scene, the main loop renders uncapped (~4800 FPS on the bare menu)
status: open
priority: high
scope: Core main loop, Graphics/Renderer frame pacing
opened: 2026-10-11
tags: [performance, power, defect]
---

# Without a scene, the main loop renders uncapped (~4800 FPS on the bare menu)

## Why

Windows-PA (RTX 3500 Ada Laptop, 2026-10-10): a jungle-ruins run whose scene failed to load idled on the bare menu
for ~165 s and logged "The rendering produced 791526 frames" — ~4800 FPS: the CPU and the GPU spin with nothing to
show. Owner decision (2026-10-11): a defect.

## What remains

- Find why neither VSync (DefaultVideoEnableVSync is true) nor Core/Video/FrameRateLimit applies in the no-scene state
  (the overlay-only frame path, the swap-chain present mode, or a loop that skips the present), and cap it.
- Measure: the bare menu on Linux / Windows / macOS — FPS at the display rate (or the limit), not thousands.
