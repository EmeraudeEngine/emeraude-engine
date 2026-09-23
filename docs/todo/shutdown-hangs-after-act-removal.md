---
id: shutdown-hangs-after-act-removal
title: Core.shutdown() takes up to a minute on NVIDIA, re-creating a scene target after removing the act
status: open
priority: unranked
scope: Core shutdown sequence, Scenes/Manager
opened: 2026-09-22
tags: [shutdown, intermittent, windows]
---

# Core.shutdown() takes up to a minute on NVIDIA, re-creating a scene target after removing the act

## Why

Windows peer session (RTX 3060 Laptop, 2026-09-22), in 1 of 2 `relief` POM runs: `Core.shutdown()` logged
"Removing the act 'relief'", then "Scene render target created" and "Scene will use environment cubemap",
and the process kept looping. A second `Core.shutdown()` connected but never executed. It left only after a
plain `taskkill` (WM_CLOSE). The other run shut down cleanly. Not seen on Linux today.

## Re-measured, 2026-09-22 (Windows, second pass)

It is SLOW, not hung. From `Core.shutdown()` to exit: 47 s (relief NM), 56 s (window-less), > 60 s (POM, which
then finished cleanly with "Core level terminated") on the RTX 3060 Laptop. It takes 2 s on the AMD iGPU of
the same machine. The long gap sits between "Removing the act" and the resource unload, and in that window
the log shows "Scene render target created" + "Scene will use environment cubemap 'AutumnFieldPureSky'":
something RE-CREATES a scene target during teardown. The first "hang" was that slowness plus a second
`Core.shutdown()` that never executed.

## Re-measured, 2026-09-23 (Windows, `relief`, RTX 3060)

`Core.shutdown()` took 2 s in most runs, but 102 s (mode 2 wireframe), 61 s (mode 2, validation on) and 179 s (mode
1, validation off). Every one of them exited cleanly in the end: the hang is intermittent and not tied to validation.

## It can hang FOREVER (2026-09-23, Windows, RTX 3060, `relief 2,32,20,0`, validation off)

The owner saw a white window on the PC: an instance that had rendered normally (screenshot and GPU timings answered)
never finished `Core.shutdown()`. Log order: "Cleanup the stage" → "Cleaning the active act 'relief'" → "Player
control unregistered" → "Removing the act 'relief'" → the Eyes/Ears devices removed → **"Scene render target created
(1280x720, R16G16B16A16_SFLOAT)"** → **"Scene will use environment cubemap 'AutumnFieldPureSky'"**, then only
`logicsTask : N ms` lines (3912 of them), never "Engine is about to stop". The console no longer executed anything,
a plain `taskkill` did nothing after 183 s, and only `taskkill /F` ended it. The two runs before it went through
the same phase in 142 s and 84 s. There was no overlapping instance.
- ⚠️ On Linux the same sequence goes from "Removing the act" straight to the resource unload: no scene target is
  created and no cubemap is assigned during teardown (checked on today's `relief` runs).
- The two lines come from `Renderer` (`recreateSceneTarget()`, on the RENDER thread when post-processing is
  active and there is no scene target) and from `Scene::enable()` / `getRenderableInstanceReadyForRendering()`.
  So the render thread keeps rendering a scene the main thread is deleting. `Stage::clean()` → `unloadActiveAct(false)`
  → `Scenes::Manager::deleteScene()`, which takes the active-scene lock exclusively and calls
  `device()->waitIdle()`.
- Hypothesis, NOT verified: a `vkDeviceWaitIdle` on the main thread while the render thread is still submitting
  (or waiting on the same lock). `vkDeviceWaitIdle` requires external synchronisation of every queue of the
  device. Check which thread holds what when it freezes: a Windows dump of both threads.

## What remains

- [ ] Reproduce it: a loop of launch → shutdown on Windows, with the log around the act removal.
- [ ] Find what re-creates a scene render target after the act is gone (a pending scene load? a queued
      switch?).
