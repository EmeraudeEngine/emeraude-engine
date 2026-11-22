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

## Fixed for the camera cut, and by construction for the scene switch (2026-11-22)

`Scene::switchToCamera()` with the motion blur on streaked the FIRST frame after the cut, every time (projet-alpha
`city`, a car's chase camera cut to its cockpit camera at 13 m/s: that frame 16.9 / 255 from the next, the following
ones 0.6-0.8 from one another). The view matrices now break their frame history when another camera feeds the target
or when the scene is another one (`ViewMatricesInterface::latchFrameHistory()`), and TAA, the GI denoiser, the
volumetric light, the depth of field's focus and the motion blur start over on that frame: graphics doc 13-12 § 43 —
The VIEW discontinuity, with the measurements (the cut frame 0.68 / 255 from the next at 13 m/s, 0.57 parked).

## What remains

- **The scene switch itself, on Windows**, where it was seen: `--load-demo default`, then `Stage.loadDemo("sponza")`,
  with `Renderer.temporalCapture` around the switch — its JSON must show `"viewDiscontinuity": true` on the first
  frame of the new scene, and that frame must be sharp. On Linux the switch never smeared, before or after.
- macOS and Windows: the camera cut's measurement of doc 43 (`city`, the car's views).

## References

- Captures on the Windows machine: projet-alpha `captures/1792168050.png` (smeared), `1792168061.png` (sharp).
