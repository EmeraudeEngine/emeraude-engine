---
id: sprite-texture-clamp-to-edge
title: Check sprite texture clamping to edges
status: open
priority: unranked
scope: Graphics/Sprite
opened: unknown
tags: [sampler]
---

# Check sprite texture clamping to edges

## What remains

- [ ] Verify the sampler addressing used by sprites: an edge texel bleeding around the quad is the
  usual symptom of a wrong wrap mode.

## ⚠️ Related

The engine already paid for a sampler-cache key collision once: `Renderer::getSampler()` used to
key on the identifier STRING and skip the setup lambda on a hit, so an explicitly requested
`CLAMP_TO_BORDER` was dead code (fixed — the key is now a hash of the `VkSamplerCreateInfo`
contents). Before concluding anything here, check which sampler the sprite really gets.

## Origin

Inherited from the historical root `TODO.md` (no date, no measurement).
