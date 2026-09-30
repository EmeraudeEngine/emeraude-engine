---
id: imposter-bake-records-destroyed-image-after-demo-switch
title: After citadel then beams then terrain, terrain's imposter bake records a command buffer whose image was destroyed
status: open
priority: unranked
scope: Graphics ImposterAtlas / imposter bake target, Scenes::Toolkit::bakeTreeImposter()
opened: 2026-09-30
tags: [vulkan, imposter, lifetime, validation]
---

# After a demo switch, terrain's imposter bake records a command buffer whose image was destroyed

## Why

Linux (RTX 3070 Ti), validation on, 2026-09-30: `--load-demo citadel`, then `Stage.loadDemo(beams)`, 20 s, then
`Stage.loadDemo(terrain)`, 40 s → 20× `VUID-vkEndCommandBuffer-commandBuffer-00059` ("VkCommandBuffer … is invalid
because bound VkImage … was destroyed"), with `[RendererService] Unable to finish the command buffer for render target
'terrain/ImposterBake_…'` ×5. `terrain` alone: 0 VUID. It predates the surface-geometry retirement fix (reproduced
with and without it).

Hypothesis (NOT verified): both citadel and terrain bake imposters of the same trees; `Toolkit::bakeTreeImposter()`
names its material and mesh `"Imposter/" + label` through `getOrCreateResourceSync()`, so the second scene gets the
FIRST scene's cached resources (pointing at the first scene's atlas) while it creates and bakes a new atlas.

## What remains

1. Confirm with gdb on `DebugMessenger::debugCallback if $rdi == 0x1000` which image and who destroyed it.
2. Fix at the root (per-scene imposter resource names, or a shared atlas owned by the resource, not the scene).

## References

- `src/Scenes/Toolkit.cpp` `bakeTreeImposter()`, `src/Graphics/ImposterAtlas.*`, the scene's `imposterBakeTarget()`.
