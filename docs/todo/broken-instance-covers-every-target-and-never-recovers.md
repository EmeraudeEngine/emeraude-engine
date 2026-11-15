---
id: broken-instance-covers-every-target-and-never-recovers
title: A failed shader program breaks its instance for every render target, for good — and a failed final pass leaves no picture
status: open
priority: high
scope: Graphics/RenderableInstance (BrokenState), Scenes/Scene.rendering (the per-frame checks), Graphics/Renderer (the final post-processing pass)
opened: 2026-11-15
tags: [defect, design, shaders, measured]
---

# A failed shader program breaks its instance for every render target, for good — and a failed final pass leaves no picture

## Why

Three behaviours seen on 2026-11-15 while a shader program was made to fail on purpose
(`--inject-shader-compilation-failure=NAME`, graphics doc 04 § *A failed program is remembered*). The failure itself is
now reported once and not retried; what the engine does NEXT is the subject here.

1. **One broken flag for every render target.** `RenderableInstance::Abstract::BrokenState` is a single bit. The scene's
   per-frame checks (`Scene::checkRenderableInstanceForShadowCasting()`, `checkRenderableInstanceForRendering()`) call
   `setBroken()` when an instance cannot get ready for THEIR target, and both refuse a broken instance that is not
   ready yet (an instance already ready for a target keeps being drawn there: the ready test comes first). Measured
   (`light-and-shadow-debug`, `ShadowCastingVertexShader` failing, before the trace was made once): the floor, broken
   by its shadow program, was refused for the main view at every frame — 2 790 lines "Unable to get ready the
   renderable instance (Renderable:LightenGroundSceneFloor') for rendering with render-target 'SceneRenderTarget'" in
   18 s. The shadow pass runs first in a frame, so an instance is broken before the main view ever prepares it: it
   leaves the picture because it cannot cast a shadow. The picture itself was not looked at.
2. **Nothing clears it.** No code path resets `BrokenState` (a search for the flag finds its setters and its readers
   only). `Core.RendererService.retryFailedPrograms()` forgets the failed programs, but a broken instance never asks
   for its program again: it stays out until its entity is destroyed.
3. **A failed final pass leaves no picture.** `PostProcessor::executeDirectPostProcessEffects()` draws the final image;
   when its program fails, `Renderer` only traces "Post-processor unable to execute the direct effects!" (its own
   `TODO: What we need to really do here, when it occurs?`) and the frame is presented without it.

## What remains

Decisions (owner), then the code:
- What a failure disables: the instance on the one target whose program failed (a per-target or per-program-type state)
  or the whole instance as today. A failed shadow program could mean "does not cast a shadow" and nothing more.
- What brings it back: `retryFailedPrograms()` clearing the broken state of the active scene's instances, an automatic
  retry when the program key changes, or nothing.
- The final pass: a fallback that shows the scene without the lens effects (a fixed, always-valid program), a message
  on screen, or a stop of the run as for a device loss.
- Then measure with the injection: the instance still drawn in the main view when only its shadow program fails; the
  picture kept when the final pass fails; a retry that brings a broken instance back.

## ⚠️ Traps

- The injection needs an empty `--cache-directory` and a tree configured with `-DEMERAUDE_FAULT_INJECTION=On`; `default`
  compiles no shadow-casting program on a ray-tracing device (its shadows are traced): use `light-and-shadow-debug`.
- A shader served by the binary cache never fails: the names to inject are those the log prints between parentheses.

## References

- `docs/subsystems/graphics/04-4-caching-architecture.md` § *A failed program is remembered* (the measures, the retry).
- `docs/subsystems/source-tree/08-startup-fault-injection.md` (`--inject-shader-compilation-failure`).
