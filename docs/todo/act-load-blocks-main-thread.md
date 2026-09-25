---
id: act-load-blocks-main-thread
title: A built-in act is loaded on the main thread, which stops answering the window system for its whole duration
status: open
priority: unranked
scope: projet-alpha Stage/Loader, engine Core (main loop)
opened: 2026-09-25
tags: [wayland, loading, responsiveness]
---

# A built-in act is loaded on the main thread, which stops answering the window system for its whole duration

## Why

`Application::onCoreStarted()` → `Stage::loadBuiltInScene()` → `Loader::load()` builds the whole scene on the
main thread, before the main loop: 9 s for `terrain` (750 000 trees, 3.2 s of tree growth alone). During that time
nothing polls the window. It is what let GNOME drop the Wayland connection (fixed the other way: the render thread
now reads the display, `Window::drainDisplayConnection()`, `docs/caution-points.md` § *GNOME dropped the Wayland
connection*), and it still leaves the compositor's `xdg_wm_base.ping` unanswered: GNOME may dim the window as
"not responding" during a long load, and no input is taken.

## What remains

- Load the act on a worker while the main loop runs (a loading state the stage shows), then enable the scene;
  or pump the window events between the loader's steps — the second is simpler but runs input callbacks in the
  middle of a load (re-entrancy to check).
- Owner decision needed between the two.
