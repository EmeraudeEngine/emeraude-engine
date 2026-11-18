---
id: overlay-transition-buffer-recreated-at-every-resize
title: A hidden overlay surface's transition buffer is recreated at every resize, with a warning that calls equal sizes divergent
status: open
priority: high
scope: src/Overlay/Surface.cpp (the transition buffer resize, around the "diverges from the content provider's painted size" warning)
opened: 2026-11-18
tags: [overlay, resize, log-noise]
---

# A hidden overlay surface's transition buffer is recreated at every resize, with a warning that calls equal sizes divergent

## Why

Measured 2026-11-18 (Linux, RADV, Wayland, a downstream application with several web-view surfaces): during 10 minutes
of window resizes (~14 000), about 2 400 times:

```
[Warning][OverlaySurface] Surface '<a hidden web view>' transition buffer size (2x1px) diverges from the content
provider's painted size (2x1px). Recreating the transition buffer at the painted size.
```

The two sizes are EQUAL. In `Surface.cpp` the early return is
`m_transitionBuffer.isValid() && m_transitionBuffer.matchesSize(requestedWidth, requestedHeight)`: the warning is
reached when the transition buffer is **invalid** (released, never created), and it prints the width and height the
invalid buffer still reports. So:

- the message is wrong for that case (it says "diverges" when the buffer is simply missing), and
- a surface kept at 2x1 px (a hidden view) gets its transition buffer recreated at every resize request — an
  allocation and a pixmap per resize, for a surface nobody sees.

## What remains

1. Find why the 2x1 surface's transition buffer is invalid at each request (released on resize, never kept for a
   hidden surface, or invalidated by the overlay's resize pass).
2. Keep a valid buffer that already matches the size, or skip the transition buffer for a hidden surface.
3. Split the trace: an invalid buffer recreated is not a size divergence (a different, quieter message — or none).
4. Re-run a resize storm: the warning must be gone, and no allocation per resize for a hidden surface.

## References

- `src/Overlay/Surface.cpp`, the transition buffer resize (`matchesSize()`, `fitsDeviceLimits()`).
- Measured by the secondary Linux machine's session, 2026-11-18 (RADV RENOIR, Mesa 26.0.8, GNOME 50).
