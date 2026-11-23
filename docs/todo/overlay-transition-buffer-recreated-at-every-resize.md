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

## The sequence (found 2026-11-19 from the run's journal)

The surface was NOT hidden: the downstream application creates it at 0.1 % of the window (`[0, 0, 0.001, 0.001]`)
and leaves it VISIBLE until first use, so it oscillated between 1x1 and 2x1 px with the window size (1280/1024 vs
1700 px wide, scale 1). Over the 10-minute storm: 6 766 transitions committed, 3 048 "came back to its active size",
2 417 warnings (1 766 2x1/2x1, 630 1x1/1x1, 21 with really different sizes). The loop:

1. the window size flips the surface to the other size before the previous transition was committed;
2. `updatePhysicalRepresentation()` sees the ACTIVE size again: it retires the transition buffer (now invalid, its
   pixmap still reporting the abandoned size) and re-syncs the provider (`onActiveBufferReady()`);
3. a frame the provider painted at the abandoned size arrives, matches no buffer, and requests that size;
4. `recreateTransitionBufferToRequestedSize()` finds the buffer invalid, prints "diverges" with the stale pixmap
   size, and allocates a new buffer.

Done on 2026-11-19: a HIDDEN surface (`hide()`) no longer follows the size changes at all (`processUpdates()` defers
Steps 1 and 1.b until `show()`; overlay doc 02, "A hidden surface does not follow the size changes"). The downstream
application was asked to hide its views with `hide()` instead of a tiny geometry (its own ticket).

Done on 2026-11-19 too: the trace is split. A missing (invalid) transition buffer is a Debug line ("has no transition
buffer: creating it at the content provider's painted size"); the "diverges" Warning is kept for a VALID buffer of
another size, with both real sizes.

Done on 2026-11-23: the downstream application now creates its preloaded views hidden (not by a tiny geometry) and
shows them only when needed, and the resize storm was re-run (macOS, Release, 400 window resizes in 8 s, the views never
shown, a control surface created the former way — `[0, 0, 0.001, 0.001]`, left visible — next to them). The control
committed **76** transition buffers (26 "came back to its active size"); a hidden surface committed **1**, its single
rebuild at its first show, and the other hidden one 0. No overlay warning or error names a hidden surface (all 107
warnings of the run were a visible surface following the resizes with really different sizes). A hidden surface opened
after the storm appears at the right size.

## What remains

1. Optional, owner to decide: on step 2 above, keep the abandoned buffer (not committable) instead of retiring it, so
   a request at that same size re-arms it without an allocation; and reuse a transition buffer that already has the
   target size in `updatePhysicalRepresentation()`. The case that produced the allocations (a visible surface that
   oscillates between two tiny sizes) no longer comes from the downstream application; it remains for any caller that
   hides a surface with a tiny geometry (the contract is in `docs/subsystems/overlay/05-development-patterns.md`).

## References

- `src/Overlay/Surface.cpp`, the transition buffer resize (`matchesSize()`, `fitsDeviceLimits()`).
- Measured by the secondary Linux machine's session, 2026-11-18 (RADV RENOIR, Mesa 26.0.8, GNOME 50).
