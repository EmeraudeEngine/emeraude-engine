---
id: gltf-transmission-texture
title: glTF — KHR_materials_transmission's texture is ignored (only the scalar factor is read)
status: open
priority: high
scope: Scenes/Loaders/GLTFLoader, Graphics/Material (StandardResource)
opened: 2026-08-25
tags: [gltf, material, defect]
---

# glTF — KHR_materials_transmission's texture is ignored

## Why

`KHR_materials_transmission` is the last supported extension that reads only its scalar factor: `transmissionTexture`
(its R channel modulates the factor per texel) is dropped. No model of the conformance bench fails on it, which is why
the bench's 20 / 20 does not cover it (`docs/subsystems/scenes-loaders/07-gltf-conformance-bench-results.md`).

## What remains

- Read `transmissionTexture` (texture info + `KHR_texture_transform`) into the material's transmission component and
  sample it where the factor is used, in both lanes.
- Prove it on a Khronos sample that has the texture, against its reference, with the bench's per-cell discipline.

## References

- Khronos `KHR_materials_transmission` specification; split out of the former item `gltf-conformance-loader-gaps`
  (2026-10-11).
