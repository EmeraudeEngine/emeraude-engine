---
id: hi-z-occlusion-culling
title: Hi-Z occlusion culling
status: open
priority: medium
scope: Graphics/Culling
opened: unknown
tags: [gpu-driven, performance]
---

# Hi-Z occlusion culling

## Why

Part of the GPU-driven roadmap. **The SSR Hi-Z pyramid already exists**; occlusion culling on top
of it does not.

## What remains

- [ ] Reuse the depth pyramid to reject occluded instances before they are drawn.

## Origin

Inherited from the historical root `TODO.md`.
