---
id: screenshot-lost-across-scene-switch
title: A screenshot requested right after a scene switch stays pending and its file is lost
status: open
priority: high
scope: Graphics/Renderer
opened: 2026-09-24
tags: [capture, console]
---

# A screenshot requested right after a scene switch stays pending and its file is lost

## Why

Seen by the Windows session (2026-09-24, NVIDIA 616.92, validation ON), after a runtime scene switch
from the projet-alpha menu (forest → Basic Scenery): the FIRST `Core.RendererService.screenshot()`,
sent about 0.5 s after the new scene became active, never wrote a file. A retry 6 s later answered
`Screenshot failed: A capture is already in progress.`; one about 10 s later succeeded. The render
thread ran normally throughout. So a capture requested across (or just after) a scene switch stays
pending for several seconds, and its image is dropped.

## Done (2026-09-24, `docs/subsystems/graphics/10-9b-frame-capture-screenshot-and-temporal-capture.md`)

- A timed-out capture is now CANCELLED instead of staying armed: the next request is no longer
  refused ("already in progress") and no late result is published to nobody.
- The stem is unique: two captures in one second no longer collide (the second one failed to write
  — reproduced on Linux; the other way to lose a capture).

## What remains

1. On Windows (the new peer machine, RTX 3500 Ada + Intel iGPU, since 2026-10-10 — the 2026-09-24 one is
   retired), with those fixes: switch scene from the menu, request a screenshot within the first
   second. If it still times out, grab the stacks of the main and render threads DURING the 5 s
   wait. On Linux a capture right after a viewer switch (`Core.openFiles`) answers in 0.3-0.6 s.
2. ⚠️ Lead: `screenshot()` runs on the MAIN thread and blocks it until the frame is written. Anything
   the render thread needs from the main thread in that window — the Windows message pump for the
   present, a swap-chain recreation waiting for a window size — cannot happen, and the capture waits
   for a frame that waits for it. Confirm or rule out with the stacks before changing the design.
3. Related fix, not this case: `c13e605c` (2026-09-30) keeps the `OnDemand` renderer drawing while a capture
   is armed (`Renderer::owesFrames()`): with NO active scene the main-thread wait starved the renderer of
   redraw requests and every capture timed out. The 2026-09-24 report HAD an active scene (which bypasses
   that gate), so it is not explained by it — but the same shape (main-thread wait, renderer starved) is
   the lead of point 2.

## References

- `Core.RendererService.screenshot()` console binding, the renderer's capture request.
