---
id: program-cache-key-codegen-inputs-audit
title: Audit the program cache key for generator inputs that change the GLSL and are not in it
status: open
priority: unranked
scope: Saphir/Generator/SceneRendering, Graphics/Shader caches
opened: 2026-09-08
tags: [shaders, cache, silent-failure]
---

# Audit the program cache key for generator inputs that change the GLSL and are not in it

## Why

`SceneRendering::computeProgramCacheKey()` (`src/Saphir/Generator/SceneRendering.cpp`) combines
eight components: render pass handle, cubemap-ness, renderable name, layer index, render pass type,
generator flags, material descriptor-set layout hash, material codegen flags. Any generator input that
changes the emitted GLSL and is not one of them makes two different shaders share one key — and both
shader caches are on by default, the SPIR-V one on disk, so the failure is silent and survives launches.

The case that exposed the class is gone: the POM layer count was a GLSL literal read from the generator
(`Core/Graphics/Texture/POMIterations`), and a 16 → 0 A/B on 2026-09-08 came out indistinguishable
(sphere region 32.50 vs 32.38 / 255) because both arms ran one cached binary. Since 2026-09-22 the count
is a material UBO value (`StandardResource::setParallaxIterations()`, `src/Graphics/AGENTS.md`
§ Parallax Occlusion Mapping) and the generator no longer has it. That item was renamed to this audit —
the half it did not cover.

## What remains

- [ ] List every value a generator reads that reaches the GLSL text (settings read in
      `SceneRendering`'s constructor, `LightGenerator` state, debug lanes such as
      `DebugMaterialPropertiesLane`), and check each against the eight key components.
- [ ] Put the missing ones in the key, or move them to a UBO value when they are per-material.

## ⚠️ Traps

- ⚠️⚠️ Until this audit is done, an A/B on a codegen setting needs both shader caches set to false.
- A key that omits a discriminating input reads exactly like an unwired feature (same family as the
  resource key built on a non-unique asset name, `87d13636`).
