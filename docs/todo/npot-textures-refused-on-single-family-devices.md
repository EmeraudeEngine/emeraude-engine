---
id: npot-textures-refused-on-single-family-devices
title: Non-power-of-two textures are refused on a device with a single queue family
status: open
priority: high
scope: Vulkan/Instance (getGraphicsDevice), Graphics/TextureResource (validateTexture)
opened: 2026-10-11
tags: [vulkan, textures, intel, defect]
---

# Non-power-of-two textures are refused on a device with a single queue family

## Why

`Instance::getGraphicsDevice()` sets `m_standardTextureCheckEnabled = m_graphicsDevice->hasBasicSupport()`
("Basic GPU do not support flexible textures"), and `hasBasicSupport()` means "only one queue family". Every texture
resource then refuses a non-power-of-two image (`TextureResource::Abstract::validateTexture()`). Vulkan 1.0 core
supports non-power-of-two images on every device, so the heuristic refuses valid data. Found on Linux with the Intel
UHD 770 (Mesa ANV, 2026-10-11): `Grounds/Dust001` (1536 × 1536) refused, `basic-scenery`'s ground lost, then a
projet-alpha crash (item `demo-ground-dereferenced-without-check`). The NVIDIA devices (several families) never take
this path; check what the Apple GPUs report.

## What remains

- Owner decision: remove the check (recommended: no Vulkan device lacks NPOT images), or tie it to a real device
  limit (`maxImageDimension2D`, format features) instead of the queue-family count.
- Re-run the Intel sweep (`--window-less`, `ForceGPU`): `basic-scenery` loads.
