---
id: rt-light-ssbo-unbound-with-disabled-light-set
title: A scene whose light set is disabled leaves the RT effects' light SSBO binding unwritten
status: open
priority: high
scope: Graphics (RT effects descriptor set) / Scenes (LightSet)
opened: 2026-09-30
tags: [vulkan, validation, ray-tracing, lighting]
---

# A scene whose light set is disabled leaves the RT effects' light SSBO binding unwritten

## Why

Seen on Linux (RTX 3070 Ti, 2026-09-30) on the `doom-loader` demo (a full-bright scene: "Lighting is not enabled
for scene 'doom-loader'"), during the triad's section 3 regression runs — independent of the triad:

- 20 × `VUID-vkCmdDispatch-None-08114` and 20 × `VUID-vkCmdDraw-None-08114`: "the descriptor [Set 0, Binding 3,
  variable "lightSSBO"] is being used … but has never been updated via vkUpdateDescriptorSets()".

`LightSet::initialize()` returns early for a DISABLED light set, before it creates the RT light SSBO; the Renderer
then writes binding 3 of the RT descriptor set only when `lightSet.RTLightBuffer()` is not null
(`Renderer.cpp`, "Binding 3: Light array SSBO"), while the RT effects (RTGI, RTR, the probes' compute) still bind
and statically use it. `rtLightCount()` is 0, so nothing is READ — but the descriptor is invalid, and a driver may
fault on it.

## What remains

- Bind a stand-in: a small always-valid SSBO owned by the Renderer (like the dummy textures), written into binding 3
  when the scene has no RT light buffer; or create the RT light SSBO even for a disabled light set (it is 8 KiB). An
  architecture choice for the owner.
- Check the same pattern for bindings 2 (materials) and 4 (sub-geometry), which are written under the same kind of
  guard.

## References

- `src/Graphics/Renderer.cpp` (RT descriptor set update), `src/Scenes/LightSet.cpp` (`initialize()`).
