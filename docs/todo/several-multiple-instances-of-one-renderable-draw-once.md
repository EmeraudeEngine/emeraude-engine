---
id: several-multiple-instances-of-one-renderable-draw-once
title: Several instanced components sharing one Renderable draw ONCE between them
status: open
priority: unranked
scope: Graphics
tags: [instancing, rendering, measured]
opened: 2026-09-22
---

# Several instanced components sharing one Renderable draw ONCE between them

## Why

A `Renderable::Abstract` is meant to be shared — that is the point of a resource. Give it to
several `Scenes::Component::MultipleVisuals`, each on its own entity with its own instance list,
and **only one of them reaches the screen**.

Measured on the projet-alpha `forest` demo, 2026-09-22, from directly above the whole terrain:

| shape | entities | components | groves on screen |
|---|---|---|---|
| 44 groves over 12 shared renderables | 44 | 132 | **4** |
| 12 groves, one distinct renderable each | 12 | 12 | **12** |

Nothing else changed between the two runs. The count that appears — four — is
`12 renderables / 3 components per grove`, i.e. **exactly one draw per renderable**.

⚠️⚠️ **It fails SILENTLY and completely.** All 44 entities are in the rendering octree, all 132
`MultipleVisuals` components are listed on their entities, no instance is marked broken, and not
one error or warning is logged. From the demo's side everything succeeded.

## What remains

Find where the per-renderable state that ought to be per-instance lives. Things already ruled out
by reading:

- The component does own its instance: `Component::MultipleVisuals` constructs its own
  `std::make_shared< RenderableInstance::Multiple >(...)` (`MultipleVisuals.hpp:73`), so the
  instance objects are distinct.
- The render list cannot lose them: `RenderBatch::List` is a `std::multimap`, duplicate keys are
  kept (`Scenes/RenderBatch.hpp:50`).
- They are not culled: the measurement is a top-down of the entire terrain, and all the groves are
  inside it.
- They are not refused as unready: `Scene::checkRenderableInstanceForRendering()` logs a broken
  instance, and nothing was logged.

So the collapse happens at or after batch insertion. The next places to look are the per-renderable
program cache (`Renderable::Abstract::findCachedProgram()` and the instance-local
`m_resolvedPrograms`), and whatever `RenderableInstance::Abstract::getReadyForRender()` stores on
the renderable rather than on the instance.

## ⚠️ Traps

- **Do not test this at eye level.** Groves hide behind each other and behind the relief; the
  count is only trustworthy from a top-down of the whole area.
- **Do not trust the scene dump.** It lists all 44 entities and all 132 components, which is the
  very reason this took a while: the scene graph is right and the frame is not.
- The `forest` demo currently **works around** it by giving every grove its own renderable, which
  caps the grove count and does not scale — a renderable is a full geometry upload. Removing that
  workaround is how to reproduce.

## References

- `src/Scenes/Component/MultipleVisuals.hpp:73`, `src/Graphics/RenderableInstance/Multiple.cpp`.
- `src/Scenes/Scene.rendering.cpp` — `populateRenderLists()`, `renderOpaque()`,
  `renderLightedSelection()`.
- projet-alpha `src/Builtin/Forest.cpp`, the grove loop and its comment.
