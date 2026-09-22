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

## What remains

- [ ] Reproduce it: a loop of launch → shutdown on Windows, with the log around the act removal.
- [ ] Find what re-creates a scene render target after the act is gone (a pending scene load? a queued
      switch?).
