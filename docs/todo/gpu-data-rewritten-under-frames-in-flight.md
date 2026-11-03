---
id: gpu-data-rewritten-under-frames-in-flight
title: Bindless slots and material data still rewritten in place while submitted frames read them
status: open
priority: high
scope: Graphics (BindlessTextureManager, Material dynamic properties, Renderer grab pass)
opened: 2026-11-04
tags: [vulkan, descriptors, frames-in-flight]
---

# Bindless slots and material data still rewritten in place while submitted frames read them

## Why

A frame SUBMITTED but not yet executed reads the GPU data as it is when the GPU runs it, not as it was when the frame
was recorded. `UPDATE_AFTER_BIND` does not freeze a submitted frame's descriptors. The IBL slots were the measured
case: one shared group, rewritten each frame, gave two frames of a day sun over a night IBL on AMD RADV (4 frames in
flight, GPU-bound). They now have one group per frame in flight (engine 4a29d706, graphics doc 06-6 § Reserved Slots;
the former item `bindless-ibl-slots-rewritten-while-in-flight`). The same class remains elsewhere:

- **The animated textures' 2D views**: `BindlessTextureManager::syncTextureSet()` → `writeAnimatedTexture2D()`
  rewrites the slot every frame with the current frame's view. A pending frame shows a later animation frame.
- **`clearTextureSet()`** (scene disable / switch): parks the leaving scene's slots and every IBL group on the defaults
  while frames of that scene may still be pending. It also returns early, WITHOUT resetting the IBL groups, when the
  set has no 2D or cube entry.
- **The grab pass**: `Renderer::refreshGrabPass()` rewrites `GrabPassSlot` / `GrabPassDepthSlot` when it is recreated.
- **Material dynamic properties** (`StandardResource::setEmissiveStrengthValue()` and the other dynamic writes, flushed
  by `Renderer::flushMaterialVideoMemoryUpdates()`): likely the same. AMD round 7 (graphics doc 13-12 § 43, "the
  glossy lift"): the day frames recorded BEFORE a time-of-day jump, still pending, show the lamps' bulbs lit in the
  reflections (251.3 against 251.0 on 1.9 % of the pixels); not at 8 FPS. Not proven: read how the material uniform
  data is buffered (one region, or one per frame in flight) first.

## What remains

- For each case, read how it is buffered and say which frames can read it. Measure the visible ones with
  `temporalCapture(N, triggerFrame, command)` on AMD (a GPU-bound device with frames in flight; the RTX shows none).
- The fix per case is the owner's decision. The IBL's per-frame copy is one model; others are a deferred write once
  the frames that used the old value have completed, or a slot that is never reused while in flight.

## References

- Graphics doc 06-6 § Reserved Slots (the per-frame IBL groups and the in-flight rule), graphics doc 13-12 § 43,
  `docs/caution-points.md` (the corrected UPDATE_AFTER_BIND notes).
