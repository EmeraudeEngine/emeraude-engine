---
id: sprite-texture-clamp-to-edge
title: Check sprite texture clamping to edges
status: open
priority: medium
scope: Graphics/Sprite
opened: unknown
tags: [sampler]
---

# Check sprite texture clamping to edges

Priority: medium by owner decision (2026-10-13), although a defect.

## What remains

- [ ] Verify the sampler addressing used by sprites: an edge texel bleeding around the quad is the
  usual symptom of a wrong wrap mode.

**Code reading (board audit, 2026-10-13): every sprite samples with REPEAT.**
- `SpriteResource.cpp:93-138` builds the sprite material from a `Texture2D` or an `AnimatedTexture2D` and never
  sets a wrap mode.
- `AnimatedTexture2D.cpp:188-190`: the three `addressMode*` lines are commented out, so the sampler keeps the
  `Renderer::getSampler()` defaults, `VK_SAMPLER_ADDRESS_MODE_REPEAT` (`Renderer.cpp:1455-1457`).
- `Texture2D.cpp:119-135` honours the texture's own wrap modes, but they default to `WrapMode::Repeat`
  (`TextureResource/Abstract.hpp:386-387`) and only the glTF loader sets them (`GLTFLoader.cpp:2680`): a store
  sprite repeats too.
- Expected effect (not yet measured): with linear filtering, the opposite edge of the frame bleeds into the
  quad border. What remains: CLAMP_TO_EDGE for sprites (both texture kinds), proven by an edge A/B capture.

## ⚠️ Related

The engine already paid for a sampler-cache key collision once: `Renderer::getSampler()` used to
key on the identifier STRING and skip the setup lambda on a hit, so an explicitly requested
`CLAMP_TO_BORDER` was dead code (fixed — the key is now a hash of the `VkSamplerCreateInfo`
contents). Before concluding anything here, check which sampler the sprite really gets.

## Origin

Inherited from the historical root `TODO.md` (no date, no measurement).
