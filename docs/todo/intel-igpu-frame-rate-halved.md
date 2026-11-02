---
id: intel-igpu-frame-rate-halved
title: The Windows Intel iGPU runs at about half its October frame rate
status: open
priority: high
scope: Graphics (the whole frame on the Intel Raptor Lake iGPU), possibly the present mode or the settings
opened: 2026-11-02
tags: [defect, performance, intel, windows]
---

# The Windows Intel iGPU runs at about half its October frame rate

## Why

Windows-PA's Intel iGPU (Raptor Lake-S Mobile, `ForceGPU` copy, ScreenSpace lane only) measured during the lighting
lots of 2026-11-01 and 11-02:
- `reflexion-debug` and `labyrinth`: 22-24 FPS (2026-11-01);
- `city` at the quay: 12-13 FPS by day and 19-21 FPS by night (2026-11-01 and 11-02, layer on).

About 47 FPS was logged on the same machine in October on other demos. If the drop is real, it is a regression to
bisect. It may also be a difference of conditions. It was observed in passing and NOT investigated.

## What remains

- Confirm or refute it under the SAME conditions as October: same demo, resolution, settings, present mode and layer
  state. Measure without the validation layer (a performance figure is never taken with it), from `getStatus` over
  ≥ 8 s, with the machine idle.
- If it is confirmed, bisect between the October reference and the current heads (a `git archive` reference build, as
  for the cold-shutdown measurements), and attribute the GPU time per pass (`GPUProfiler`).

## ⚠️ Traps

- engine item `auto-exposure-fp16-dead-band` records "~47 FPS (Intel FIFO, or `FrameRateLimit` 48)". The October figure
  may be a present-mode cap, not the iGPU's throughput. Check which one it was before calling anything a regression.
- Every figure above was taken with the validation layer ON.

## References

- Graphics doc 42 § "The environment" (the Intel stage cost of the sky's IBL); graphics doc 13-12 § 16 (the
  peers' runs of 2026-11-02).
