---
id: transmission-ignores-metalness
title: A metallic transmissive material transmits like a dielectric
status: open
priority: high
scope: Saphir/LightGenerator (Reflection + Transmission branch)
opened: 2026-09-22
tags: [pbr, transmission, gltf-conformance, cross-platform-bench]
---

# A metallic transmissive material transmits like a dielectric

## Why

Found by the macOS peer session on `TransmissionTest` (2026-09-22). The "Metallic Transmission" row renders
as a uniform clear sphere, where the reference shows tinted mirror stripes. From reading the code only: the
Reflection + Transmission branch of `src/Saphir/LightGenerator.cpp:1025-1066` uses a dielectric-only reflectance
(`fresnelDielectric` from `dielectricF0Expression()`) and never scales the transmission by (1 − metalness) — still
true on 2026-10-13. Only the DIFFUSE leg is already weighted by (1 − metalness) · (1 − transmission)
(`diffuseWeightShaderExpression()`, `:95-115`). The glTF PBR model: a metal does not transmit.

## What remains

- [ ] Confirm on Linux, then weight the transmission by (1 − metalness) with the metal's F0 reflectance,
      as `KHR_materials_transmission` specifies.

The missing stripes of the red TransmissionTexture spheres on the same test are a separate item:
`gltf-transmission-texture`.
