---
id: material-texture-canonical-repack
title: Repack the material textures into the canonical layout (Albedo+opacity, Normal alone, Data AO/roughness/metalness/height)
status: in-progress
priority: unranked
scope: Graphics/Material, Graphics/ImageResource, TextureCache, scene loaders
opened: 2026-10-20
tags: [material, textures, memory, texture-packing]
---

# Repack into the canonical layout

## Why

Graphics § 39: whatever the source files, the engine builds at most three textures for the core PBR maps — Albedo RGB
+ opacity A (sRGB), the normal alone, Data R AO / G roughness / B metalness / A height (UNORM).

## What remains

- DONE on Linux (2026-10-20, uncommitted): L2a (`PackRecipe`, `ImageResource::load(PackRecipe)`, base
  `Pixmap::copyChannel()` and `Processor::resampleTexture()`) and L2b (the store JSON planner,
  `Core/Graphics/Texture/Packing`), the RT opacity channel — graphics § 39 § The repack (Sand002 26.7 → 16.0 MiB,
  Grid003 4 → 3 textures, both within the noise against the unpacked render).
- The CPU-copy release and the recomposition: the packed image declares `releasesLocalData()` and recomposes in
  `reloadLocalDataFromOwnSource()`; NOT exercised yet (a reload of a released packed image, e.g. by a readback).
- L2c DONE on Linux (2026-10-20, uncommitted) through the shared `TexturePacker` (D14): FBX loader, Toolkit foliage.
  NOT exercised at runtime: no store asset reaches those paths (graphics § 39) — needs an FBX with separate maps or a
  vegetation with a `-alpha` image to be proven.
- The albedo + opacity BC7 cost (macOS: 0.66 against a 0.21 noise on Grid003's panels): KEPT by the owner
  (2026-10-20); the encoder tuning is item `bc7-quality-for-textures-with-alpha`.
- L2d: glTF (an occlusion image separate from the ORM; the albedo alpha is already in place).
- D10: ONE `texture()` per canonical texture, its components reading their channels from that fetch — then the
  frame-time A/B at a pinned pose (Ave Performus: ≥ 5 %, median of ≥ 5 runs).
- D7: the `TextureCache` key = hash of the final pixels + the package descriptor, `TextureCache::Version` bumped
  (today the pixel hash alone already tells a packed image apart).
- Validation by the peers (macOS, Windows).

## ⚠️ Traps

- The albedo is sRGB: only its A may carry linear data (opacity). Never pack linear data into its RGB.
- A packed role read by RT needs its channel in `GPURTMaterialData` first (the opacity has it since 2026-10-20).
- Upscale with `Processor::resampleTexture()`, never `resize()` (corners mapped, Black past the border).

## References

- `docs/subsystems/graphics/39-material-texture-packing.md`, `30-compressed-image-path.md`,
  `31-alpha-coverage-a-mask-is-not-a-gradient.md`, `docs/subsystems/resources/03-resources-specific-rules.md`
  § CPU Copies, base `docs/subsystems/source-tree/08-pixelfactory-thread-safety-and-resize.md`.
