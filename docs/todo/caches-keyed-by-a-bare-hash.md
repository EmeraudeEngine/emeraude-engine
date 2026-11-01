---
id: caches-keyed-by-a-bare-hash
title: The sampler cache and the combine / denoise pass variants are still keyed by a bare size_t hash
status: open
priority: high
scope: Graphics/Renderer.cpp (getSampler, m_samplers), Graphics/CombinePass.cpp and DenoisePass.cpp (m_variants), Physics/Manager.hpp (m_pipelineLayouts, m_computePipelines)
opened: 2026-11-01
tags: [vulkan, cache, defect, latent]
---

# The sampler cache and the combine / denoise pass variants are still keyed by a bare size_t hash

## Why

A cache that SERVES what it finds under a bare `size_t` hash, with no equality behind it, hands one object out for
another when two hashes meet, and says nothing. That served a spot-shadow program for the sun's cascaded pass in 1 run
in 6 to 12 (the program cache, fixed 2026-10-31 by `Saphir::Generator::ProgramKey`). The graphics pipeline cache was
the second one (keyed by `Base::ExactKey` on 2026-11-01). The owner asked, on
2026-11-01, for the rest to get an item too. No collision has been observed in these.

| Cache | Key today | Served on a collision |
|---|---|---|
| `Renderer::m_samplers` | `hashSamplerCreateInfo(createInfo)` | a sampler with other filters / address modes / LOD range |
| `CombinePass::m_variants` | `hashCombine` of each contribution's `FNV1a(prefix)`, sampler count, needs-flags | another set of contributions' combine shader |
| `DenoisePass::m_variants` | `hashCombine` of each contribution's `FNV1a(prefix)`, target format, needs-flags | another set of contributions' denoise shader |
| `Physics::Manager::m_pipelineLayouts`, `m_computePipelines` | `size_t` | nothing: only `clear()`ed, never filled (dead members) |

## What remains

- [ ] Key each live cache by `Base::ExactKey` (the bytes of every create-info input, the hash only placing it), as
  `GraphicsPipeline::getKey()` does.
- [ ] The two dead Physics::Manager members: remove them, or say what they were meant for (owner).
- [ ] Prove: the cascade builds, the sampler and variant counts unchanged on a large scene (sponza, city), 0 VUID.

## References

- Engine `docs/pipeline-caching-system.md`, `docs/caution-points.md` § "A cache keyed by a bare hash serves one program
  for another"; base `src/ExactKey.hpp`.
