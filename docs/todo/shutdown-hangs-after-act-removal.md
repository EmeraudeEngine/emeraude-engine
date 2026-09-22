---
id: shutdown-hangs-after-act-removal
title: Core.shutdown() sometimes keeps rendering after removing the act
status: open
priority: unranked
scope: Core shutdown sequence, Scenes/Manager
opened: 2026-09-22
tags: [shutdown, intermittent, windows]
---

# Core.shutdown() sometimes keeps rendering after removing the act

## Why

Windows peer session (RTX 3060 Laptop, 2026-09-22), in 1 of 2 `relief` POM runs: `Core.shutdown()` logged
"Removing the act 'relief'", then "Scene render target created" and "Scene will use environment cubemap",
and the process kept looping. A second `Core.shutdown()` connected but never executed. It left only after a
plain `taskkill` (WM_CLOSE). The other run shut down cleanly. Not seen on Linux today.

## What remains

- [ ] Reproduce it: a loop of launch → shutdown on Windows, with the log around the act removal.
- [ ] Find what re-creates a scene render target after the act is gone (a pending scene load? a queued
      switch?).
