---
id: pom-setting-outside-program-cache-key
title: POMIterations changes the generated GLSL but not the program cache key
status: open
priority: high
scope: Saphir/Generator/SceneRendering, Graphics/Shader caches
opened: 2026-09-08
tags: [shaders, cache, measured, silent-failure]
---

# POMIterations changes the generated GLSL but not the program cache key

## Why

`SceneRendering::computeProgramCacheKey()` (`src/Saphir/Generator/SceneRendering.cpp:902`) combines
**eight** components: render pass handle, cubemap-ness, renderable name, layer index, render pass
type, generator flags, material descriptor-set layout hash, material codegen flags.

**`pomIterations()` is not one of them** — and it changes the emitted GLSL structurally. Per
`src/Saphir/AGENTS.md`: at 0 "no POM code in shaders, no extra vertex outputs"; above 0 the
march is emitted with the count baked in. So two structurally different shaders share one cache key.

Both shader caches are **on by default** and the SPIR-V one is on disk, surviving between runs
(engine `docs/caution-points.md`; measured speedups SPIR-V ×38, `VkPipelineCache` ×182). The
failure is therefore silent and durable: change the setting, relaunch, and the old SPIR-V is served.

## Measured, 2026-09-08

On `light-and-shadow-debug`, pinned pose `setPosition(-4, 2, 8)` / `lookAt(-4, 2, 1)`, pinned
exposure, two launches of the same binary differing ONLY by
`Core/Graphics/Texture/POMIterations` 16 → 0: the sphere region came out at mean **32.50** vs
**32.38** / 255 (max 116 vs 123). Indistinguishable — which is what a shared cache entry produces
**by construction**, so that measurement proves nothing about POM and must not be read as "POM has
no effect". It is the symptom of this defect.

⚠️ `highQualityEnabled()` is NOT the explanation: `Saphir/Generator/Abstract.hpp:188` states
"nothing drives it down yet, so it is currently always true" for scene-rendering programs.

## What remains

- [ ] Add the POM iteration count to `computeProgramCacheKey()`.
- [ ] Audit the other seven components for the same class of gap: any generator input that changes
      the emitted GLSL and is not in the key.

## ⚠️ Traps

- ⚠️⚠️ **Any A/B on a codegen setting is invalid until this is fixed.** Set both shader caches to
  false for the measurement, or the two arms share one binary.
- ⚠️ **Documentation divergence, found the same day.** `src/Saphir/AGENTS.md` § *POM Iterations
  Setting* documents the key as `Core/Graphics/Shader/POMIterations` with **default 16**. The code
  has `GraphicsTexturePOMIterationsKey{"Core/Graphics/Texture/POMIterations"}` with
  `DefaultGraphicsTexturePOMIterations{0}` (`SettingKeys.hpp:406`). The key MOVED and the default
  changed; the doc never followed. Consequence observed on the owner's machine: a hand-tuned
  `Core/Graphics/Shader/POMIterations = 16` sat in `settings.json` and **was no longer read by
  anything**. Fix the doc in the same pass.
- Same defect family as the resource key built on a non-unique asset name (`87d13636`): a key that
  omits a discriminating input reads exactly like an unwired feature.
