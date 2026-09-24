---
id: screenshot-lost-across-scene-switch
title: A screenshot requested right after a scene switch stays pending and its file is lost
status: open
priority: unranked
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

## What remains

1. Reproduce (any OS): switch scene, request a screenshot within the first second, then retry.
2. Read how the pending capture is tied to a render target or a frame of the previous scene, and
   what completes or cancels it when the scene (and its render targets) change.
3. Fix it in the capture path: either the pending request survives the switch and is served by the
   next frame, or it fails at once with a reason — never a silent loss followed by "already in
   progress".

## References

- `Core.RendererService.screenshot()` console binding, the renderer's capture request.
