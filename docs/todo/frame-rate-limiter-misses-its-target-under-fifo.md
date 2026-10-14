---
id: frame-rate-limiter-misses-its-target-under-fifo
title: FrameRateLimit misses its target under FIFO (macOS 26, Windows Intel 22 for 30): the software limiter is relative to the frame start
status: open
priority: high
scope: Graphics/Renderer (applyFrameRateLimit)
opened: 2026-10-14
tags: [renderer, pacing, macos, windows, defect]
---

# FrameRateLimit misses its target under FIFO (macOS 26, Windows Intel 22 for 30): the software limiter is relative to the frame start

## Why

macOS peer (M6, MoltenVK FIFO only, 60 Hz display), engine 7e34e37d, bare menu, `Core/Video/FrameRateLimit = 30` on a
settings copy: **25-26 FPS**, frame times 33-42 ms (average 38.6-39.7 ms), with and without the validation layer. The
no-scene pacing stands down under the limiter (ecc9a246), so this is the limiter alone. Linux (RTX 3070 Ti, FIFO,
60 Hz, same setting): 30-31 FPS. Windows peer, Intel iGPU (FIFO only, 165 Hz panel), bare menu, `FrameRateLimit = 30`:
**22.0 FPS — the same before the 2026-10-13 pacing (900846fe)**, so pre-existing: 1/30 s is 5.5 vblanks at 165 Hz and
a relative wait lands on ~7.5 (NVIDIA MAILBOX on the same machine: 30.0).

Reading (peer, not instrumented): `Renderer::applyFrameRateLimit()` sleeps until `m_frameStartTime + m_frameDuration`,
a deadline RELATIVE to each frame's start (taken in `beginFrame()`), not an absolute one advancing by the period. With
a blocking present the start drifts against the vertical sync, and a fraction of frames wait a third vblank (50 ms):
a mix of 33 and 50 ms frames averages ~39 ms. Why Linux lands on 30 and macOS does not is not established (present
timing of MoltenVK vs the NVIDIA driver).

## What remains

- Instrument on macOS (frame start, present return, sleep) to confirm the drift.
- Candidate (owner decision): an absolute deadline advancing by the period, a late frame not sleeping — the rule the
  no-scene pacing now follows (`Core::renderingTask()`, ecc9a246). Re-measure MAILBOX and FIFO on the three OS, scene
  and bare menu.

## References

- engine `docs/caution-points.md` § "without a scene, the Continuous loop rendered uncapped" (the two 2026-10-14
  regressions and their rule).
