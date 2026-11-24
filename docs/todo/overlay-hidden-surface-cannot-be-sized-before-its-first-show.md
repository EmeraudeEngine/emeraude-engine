---
id: overlay-hidden-surface-cannot-be-sized-before-its-first-show
title: A hidden overlay surface cannot take its size before its first draw, so a hidden view has to be rendered ahead of time
status: open
priority: low
scope: src/Overlay/Surface.cpp (processUpdates steps 1 and 1.b, show()), UIScreen::render
opened: 2026-11-24
tags: [overlay, hidden, lazy]
---

# A hidden overlay surface cannot take its size before its first draw, so a hidden view has to be rendered ahead of time

## Why

A hidden surface defers its size changes to `show()` (`processUpdates()` steps 1 and 1.b): it keeps the size it was created
with. A content provider that would render **on demand** — when the surface is shown — therefore cannot be used: the first
frame at the final size needs the surface's own resize transition, and the surface is drawn stale in the meantime.

Measured on a downstream application (macOS, Release, a web page preloaded in a hidden surface, first show, a burst of
screenshots ~170 ms apart): showing at once with frames dropped while hidden gives black for ~0.7 s and clean content at
~0.9 s; deferring the show to the first frame the provider presents gives a black strip (~17 % of the area) until ~1.7 s and
clean content at ~2.2 s. With the buffer already at its final size, neither was needed.

So the application renders its hidden views ahead of time instead: it keeps the frames they paint until their first show
(so the buffer holds the page), then puts them to sleep after a **fixed delay** following the page load. The cost: a fixed
delay, a stale frame in stock, hidden rendering at startup, and buffers allocated at creation (~27 MB per RGBA buffer at
3456x1950 px, `docs/subsystems/overlay/05-development-patterns.md`).

## What would remove it

One of:

1. Apply a hidden surface's pending size at `show()`, before its first draw, and tell the provider.
2. A **draw hold**: a surface that is shown (so it applies its size change) but not drawn until the first frame at its size is
   committed.

Either makes rendering on demand possible: no kept frames, no fixed sleep delay (the downstream application's delay is the
debt to remove then), buffers allocated at the first `show()`.

## ⚠️ Traps

- Resizing hidden surfaces eagerly brings back the resize-storm cost that deferring them removed (400 window resizes: 76
  transition buffers for a tiny visible surface, 1 for a hidden one, see the transition-buffer item). The size must be applied
  at show time only.
- Rendering on demand trades the black frame for a delay: the page needs a re-layout at the new size (~0.2-0.45 s measured). A
  draw hold shows nothing during it; it is not free.

## References

- `docs/subsystems/overlay/05-development-patterns.md` (the creation contract and why a hidden surface allocates at creation).
- `docs/todo/overlay-transition-buffer-recreated-at-every-resize.md` (parked; the storm measurement).
