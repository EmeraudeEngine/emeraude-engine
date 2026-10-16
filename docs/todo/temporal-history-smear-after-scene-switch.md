---
id: temporal-history-smear-after-scene-switch
title: The first frames after a scene switch may be smeared, as if the temporal history were not reset
status: open
priority: low
scope: engine post-processing (temporal history: TAA, motion blur), Scenes::Manager scene switch
opened: 2026-10-16
tags: [rendering, post-process, temporal, scene-switch]
---

# The first frames after a scene switch may be smeared, as if the temporal history were not reset

## Why

Seen once by Window-PA (2026-10-16, Windows, RTX 3500 Ada, Release, engine fc6e4692): projet-alpha `--load-demo
default`, then `Stage.loadDemo("sponza")`; a screenshot 1 s after the switch showed the whole image heavily smeared
(radial and vertical streaks, as if motion blur or the temporal accumulation were computed against the PREVIOUS
scene's camera and history). Six seconds later (the menu capture) the scene was sharp. Owner ranking: low (2026-10-16).

Not reproduced on Linux the same day (RTX 3070 Ti): the capture 1 s after the same switch was sharp — the timing of
the first presented frames differs between the machines, so it is not ruled out.

## What remains

- Find what holds a temporal history across a scene switch (TAA history, motion vectors' previous view-projection,
  motion blur, any accumulation of the post-processor) and whether the switch invalidates it.
- Reproduce on Windows with screenshots every 100 ms after the switch (the smear is transient), then on Linux.
- If confirmed: reset every temporal history at a scene switch (and at a camera cut), traced.

## References

- Captures on the Windows machine: projet-alpha `captures/1792168050.png` (smeared), `1792168061.png` (sharp).
