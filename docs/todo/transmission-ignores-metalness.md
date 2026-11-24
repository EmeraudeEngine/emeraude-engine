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

## Confirmed on Linux (2026-11-24, RTX 3070 Ti, engine `develop` 74beadf5)

`TransmissionTest.glb` opened in the model viewer (`Core.openFiles()`, no scene loaded, window-less), beside the
sample's own reference picture (`Models/TransmissionTest/screenshot/screenshot_large.png`): in the "Metallic
Transmission" row the yellow sphere is a uniform clear sphere, like the one of the "Transmission" row above it, where
the reference shows mirror stripes. The first step below is done: the defect is on Linux as on macOS.

## What remains

- [ ] Confirm on Linux, then weight the transmission by (1 − metalness) with the metal's F0 reflectance,
      as `KHR_materials_transmission` specifies.

The missing stripes of the red TransmissionTexture spheres on the same test are a separate item:
`gltf-transmission-texture`.
