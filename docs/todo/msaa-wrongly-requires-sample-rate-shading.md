---
id: msaa-wrongly-requires-sample-rate-shading
title: Device selection refuses a GPU without 'sampleRateShading' when MSAA is enabled, though MSAA does not need it
status: open
priority: high
scope: engine Vulkan/Instance (device selection), MSAA as an anti-aliasing option
opened: 2026-11-10
tags: [vulkan, msaa, anti-aliasing, device-selection]
---

# Device selection refuses a GPU without 'sampleRateShading' when MSAA is enabled, though MSAA does not need it

## Why

`Instance::checkDevicesFeaturesForGraphics()` (src/Vulkan/Instance.cpp) refuses a candidate device, with an Error
("MSAA is enabled in settings ! … cannot perform multisampling !"), when `Core/Video/Framebuffer/Samples` > 1 and the
device does not advertise the Vulkan 1.0 feature `sampleRateShading`. The check carries the author's own
`// TODO: Maybe incorrect assertion !`.

It is incorrect:

- **MSAA does not need `sampleRateShading`.** Multisampled rasterization (`rasterizationSamples` > 1, multisampled
  attachments, a resolve) is core Vulkan 1.0 and is bounded only by the device LIMITS
  `framebufferColorSampleCounts` / `framebufferDepthSampleCounts`. `sampleRateShading` only enables SAMPLE SHADING
  (`VkPipelineMultisampleStateCreateInfo::sampleShadingEnable` / `minSampleShading`: the fragment shader runs per sample
  instead of per pixel).
- **The engine never enables sample shading**: `GraphicsPipeline.cpp` always sets `sampleShadingEnable = VK_FALSE`,
  `minSampleShading = 1.0F`.
- **The sample count is already validated where it matters**: `Device::checkMultisampleCount()` clamps the requested
  count to the highest one both attachment kinds support (`PhysicalDevice::supportedFramebufferSampleCounts()`), and
  disables multisampling below Vulkan 1.2. A device with no multisampling at all is handled there, not refused.

So a device lacking only `sampleRateShading` is refused for nothing whenever MSAA is enabled. The default (`1` sample)
hides it.

## Owner context (2026-11-10)

Modern anti-aliasing (TAA and its family) no longer relies on multisampling, but **MSAA must stay a possible option for
specific cases**. The fix keeps MSAA available; it must not quietly remove it.

## What remains

1. **Remove the refusal**: delete the `sampleRateShading` block of `checkDevicesFeaturesForGraphics()`. The feature
   still appears in the one-line Info census of absent core features, which is enough. Update the `@note` of the
   declaration in `Instance.hpp`.
2. **If sample shading is ever used** (e.g. a specific MSAA quality mode), request `sampleRateShading` where it is used,
   through `requestOptionalVK10` in `createGraphicsDevice()` with the capability lost named, and disable that mode, not
   the device.
3. **Check that MSAA still works end-to-end** with `Samples` > 1 on the current render stack (G-buffer, post-process,
   TAA, the resolve). Test with a settings COPY and the validation layer: 0 `VUID-`, and look at the image. MSAA has not
   been exercised since the default became `1`. If a stage does not support it, that is a separate item, and the owner
   decides MSAA's supported scope (which passes, combined with TAA or exclusive).
4. **Document** MSAA's status as an option (when to use it, what it costs, what it combines with) in the topic doc, and
   record the corrected device-selection rule in `docs/caution-points.md`.

## ⚠️ Traps

- Do not replace the refusal with a check of `supportedFramebufferSampleCounts()` here: `checkMultisampleCount()`
  already clamps, and a device must not be refused for a setting the engine can lower.
- `Core/Video/Framebuffer/Samples` is a launch-time setting (`FramebufferPrecisions` reads it). Prove a test reads the
  value you set; never edit the owner's `settings.json`.

## References

- Vulkan spec: `VkPhysicalDeviceFeatures::sampleRateShading`, § Sample Shading; `VkPhysicalDeviceLimits::framebufferColorSampleCounts`.
- `src/Vulkan/Instance.cpp` (`checkDevicesFeaturesForGraphics`), `src/Vulkan/Device.cpp` (`checkMultisampleCount`),
  `src/Vulkan/PhysicalDevice.hpp` (`supportedFramebufferSampleCounts`, `highestSampleCountAtMost`),
  `src/Vulkan/GraphicsPipeline.cpp` (multisample state), `src/Graphics/FramebufferPrecisions.hpp`.
