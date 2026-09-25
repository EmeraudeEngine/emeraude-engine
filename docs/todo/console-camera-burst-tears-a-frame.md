---
id: console-camera-burst-tears-a-frame
title: A burst of console lookAt() renders an occasional single tilted, motion-blurred frame
status: open
priority: unranked
scope: Console, Scenes (camera / player nodes), logic-to-render state publication
opened: 2026-09-25
tags: [console, camera, threading]
---

# A burst of console lookAt() renders an occasional single tilted, motion-blurred frame

## Why

Measured on 2026-09-25 while benching RushMaker on `forest` (validation ON, hardware H.265 rush, 30 FPS):
`Core.SceneManagerService.Act.lookAt()` sent ~50 times per second for 12 s (a slow 80° yaw pan, the target
at the eye's height + 0.24 m) produced **3 frames out of 366** (86, 103, 116) whose horizon is visibly
TILTED and motion-blurred, the next frame back on the pan. The recorder copies inside the frame since that
day, so these frames were rendered this way — not a capture artefact. A mouse-driven rush of the same scene
(328 frames) showed none; the same burst at ~10 Hz on the VP9 path showed none either.

A roll must not exist: `lookAt()` keeps the body upright (yaw on the body, pitch on the head).

## What remains

- Reproduce on screen with a screenshot burst, then read the published camera state of the bad frame
  (`getStateSyncStatistics(true)`, the node orientations around the frame).
- Hypothesis to verify, not a finding: the console runs on the MAIN thread and writes the body yaw and the
  head pitch as two node updates while the logic thread publishes the render state — a publication between
  the two, or a motion-vector frame built across them, would give one frame of a combined odd orientation
  and a large camera velocity (the blur).

## ⚠️ Traps

- Do not bench anything with a console camera burst until this is understood: drive the camera with the
  mouse, or use a demo's own motion.
