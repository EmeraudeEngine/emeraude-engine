---
id: transmission-ignores-metalness
title: A metallic transmissive material transmits like a dielectric
status: open
priority: unranked
scope: Saphir/LightGenerator (Reflection + Transmission branch)
opened: 2026-09-22
tags: [pbr, transmission, gltf-conformance, cross-platform-bench]
---

# A metallic transmissive material transmits like a dielectric

## Why

Found by the macOS peer session on `TransmissionTest` (2026-09-22). The "Metallic Transmission" row renders
as a uniform clear sphere, where the reference shows tinted mirror stripes. From reading the code only: the
Reflection + Transmission branch of `LightGenerator.cpp` (~892-932) uses a dielectric-only reflectance and
never scales the transmission by (1 − metalness). The glTF PBR model: a metal does not transmit.

## What remains

- [ ] Confirm on Linux, then weight the transmission by (1 − metalness) with the metal's F0 reflectance,
      as `KHR_materials_transmission` specifies.
- [ ] Re-judge the MASK spheres' interiors (macOS saw them as untinted upside-down mirrors). It was
      suspected to share the cause of the back-face defect, fixed 2026-09-22 (`caution-points.md` § A
      double-sided back face reflected as an untinted mirror); check whether they are right now.
- [ ] The known gap on the same test (the missing stripes of the red TransmissionTexture spheres) is
      separate.
