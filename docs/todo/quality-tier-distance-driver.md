---
id: quality-tier-distance-driver
title: Drive the shader quality tier from rendering distance
status: open
priority: medium
scope: Saphir/Generator
opened: unknown
tags: [shaders, performance]
---

# Drive the shader quality tier from rendering distance

## Why

`EnableHighQuality` is gone as a user setting (Aug 2026). The tier is now a rendering decision
carried by `Saphir::Generator::Abstract`'s `HighQualityEnabled` flag, and `SceneRendering` enables
it **unconditionally** — every program is generated at FULL quality today.

What it gates: Fresnel-gated reflection, thin-surface transmission with Fresnel, and parallax
occlusion mapping. Also, in `src/Graphics/Material/StandardResource.cpp` (`generator.highQualityEnabled()`, l.3239-4683):
the bindless environment-cubemap reflection / refraction / transmission sampling (l.3575-3793), the clear-coat normal's
environment sample (l.4481), the grab-pass screen-space transmission (l.4592) and the water column it measures
(depth-based opacity and shore foam, l.4683) — a distance switch changes water and transmission too.

## What remains

- [ ] Drive it from rendering DISTANCE: a distant surface takes the cheap branches.

The flag is **already part of the program cache key**, so a distance switch produces its own
program variants for free.

## Note

The former "PBR low-quality specular approximation" item is void: `lqSpecPower` lived in
`generateFinalFragmentOutput()`, written for the Gouraud path, and was deleted with it — it had no
caller left.

⚠️ Before the flag goes down anywhere: a glass generated at low quality loses its refraction in the ambient pass
(item `low-quality-pbr-glass-loses-refraction`) — fix it first.
