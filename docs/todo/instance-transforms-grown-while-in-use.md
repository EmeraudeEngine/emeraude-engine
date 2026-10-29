---
id: instance-transforms-grown-while-in-use
title: The instance-transforms SSBO is grown and its descriptor set rewritten while a command buffer still uses them
status: open
priority: high
scope: Scenes/SceneInstanceTransforms (updateVideoMemory, ensureCapacity), Scenes/Scene.rendering.cpp
opened: 2026-10-29
tags: [vulkan, synchronization, defect, measured]
---

# The instance-transforms SSBO is grown and its descriptor set rewritten while a command buffer still uses them

## Why

projet-alpha `city 128,1,12,0` with the validation layer (Linux, 2026-10-29, engine cc61b294; the same on alpha
2deb0f26 and on the buildings work after it): night (`CelestialClock.setTimeOfDay(CitySun0, CitySunClock, 0.95)` +
`stop`), `Core.togglePhysicalSimulation()` (off), then the camera teleported to an aerial view
(`Act.setPosition(0, 300, -1300)`, `lookAt(0, 0, 0)`) — 38 VUIDs:
- 18 × `VUID-vkUpdateDescriptorSets-None-03047`: "pDescriptorWrites[0].dstBinding (0) was created with
  VkDescriptorBindingFlags(0), but VkDescriptorSet … is in use by VkCommandBuffer …";
- 20 × `VUID-vkCmdDrawIndexed-None-08114`: "the storage buffer descriptor [Set 1, Binding 0, variable
  "ubInstanceTransforms"] is using buffer … that is invalid or has been destroyed".

The teleport makes many more instances visible at once: `SceneInstanceTransforms::updateVideoMemory()` grows the
frame's SSBO (`ensureCapacity()`, the old one retired through the deferred destructor) and rewrites the frame's descriptor
set (`writeBufferToDescriptorSet()`). Its comment states "the frame-in-flight fence guarantees no in-flight command
buffer references them" — the validation layer disagrees: the set is still bound by a recorded command buffer. Without
the teleport (the same city at the spawn) the runs had 0 VUID.

## What remains

- [ ] Find which command buffer still holds the set: one recorded EARLIER in the same frame (a shadow map, a
  render-to-texture, the imposter bake) before `updateVideoMemory()` runs, or one not fenced by this frame's fence.
- [ ] Decide the fix with the owner: grow before ANY recording of the frame; or never rewrite a set in use (a new set
  with the new buffer, the old one retired); or reserve the capacity from the scene's instance count.
- [ ] Prove: the same sequence, 0 VUID; TSan / sync validation on the growth path.

## References

- `src/Scenes/SceneInstanceTransforms.cpp` (`updateVideoMemory()`, `ensureCapacity()`), `Scene.rendering.cpp:939`
  (`beginFrame()`).
