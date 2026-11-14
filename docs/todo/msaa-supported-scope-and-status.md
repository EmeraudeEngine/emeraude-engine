---
id: msaa-supported-scope-and-status
title: MSAA as an option — decide its supported scope, measure its cost, document it
status: open
priority: unranked
scope: Graphics (FramebufferPrecisions, the scene target, the post-process chain, TAA), Vulkan/Device (checkMultisampleCount)
opened: 2026-11-14
tags: [msaa, documentation, measurement]
---

# MSAA as an option — decide its supported scope, measure its cost, document it

## Why

Owner context (2026-11-10): modern anti-aliasing (TAA and its family) no longer relies on multisampling, but **MSAA must
stay a possible option for specific cases**. The device-selection defect that refused a GPU for it is fixed
(2026-11-14, `docs/caution-points.md` § *Device selection: MSAA does not need `sampleRateShading`*). What was never
done is to say what MSAA is in this engine: it has not been exercised since the default became 1 sample.

First run (2026-11-14, projet-alpha `default`, Debug, RTX 3070 Ti, validation layer loaded, `Core/Video/Framebuffer/
Samples = 4` in a settings copy): the swap-chain's framebuffer precisions read 4 samples, 0 `VUID-`, 0 `[Error]`, a
clean exit. The screenshot is the same picture as with 1 sample (the horizon's edge profile is identical, the level is
about 7 % higher at an UNPINNED exposure: not attributed). So nothing breaks — and nothing in that view shows that the
4 samples do anything.

## What remains

- Find where the samples are really used: which targets are multisampled (the swap-chain's, the scene target's, the
  G-buffer's), where the resolve is, and whether the post-process chain and TAA read a resolved or a multisampled
  image. A view with geometric edges and TAA off is the measurement (`sponza`, a pinned exposure, 1 against 4 samples,
  per-pixel).
- The owner decides the supported scope: which passes, combined with TAA or exclusive.
- Measure its cost (frame time, video memory) on the 8 GB target, then document MSAA's status in the topic doc: when
  to use it, what it costs, what it combines with.
- If sample shading is ever wanted (a specific quality mode), request `sampleRateShading` where it is used
  (`createGraphicsDevice()`, `requestOptionalVK10`, the capability lost named) and disable that mode, not the device.

## ⚠️ Traps

- `Core/Video/Framebuffer/Samples` is a launch-time setting (`FramebufferPrecisions` reads it): prove a test reads the
  value it set (the log's "Framebuffer precisions data"); never edit the owner's `settings.json`.
- Do not add a refusal on `supportedFramebufferSampleCounts()` at the device selection: `checkMultisampleCount()`
  already clamps, and a device is not refused for a setting the engine can lower.

## References

- `src/Vulkan/Device.cpp` (`checkMultisampleCount`), `src/Vulkan/PhysicalDevice.hpp` (`supportedFramebufferSampleCounts`,
  `highestSampleCountAtMost`), `src/Vulkan/GraphicsPipeline.cpp` (multisample state),
  `src/Graphics/FramebufferPrecisions.hpp`.
