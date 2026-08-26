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
occlusion mapping.

## What remains

- [ ] Drive it from rendering DISTANCE: a distant surface takes the cheap branches.

The flag is **already part of the program cache key**, so a distance switch produces its own
program variants for free.

## Note

The former "PBR low-quality specular approximation" item is void: `lqSpecPower` lived in
`generateFinalFragmentOutput()`, written for the Gouraud path, and was deleted with it — it had no
caller left.
