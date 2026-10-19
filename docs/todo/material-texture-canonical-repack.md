---
id: material-texture-canonical-repack
title: Repack the material textures into the canonical layout (Albedo+opacity, Normal+height, Data ORM)
status: open
priority: unranked
scope: Graphics/Material, Graphics/TextureResource, TextureCache, scene loaders
opened: 2026-10-20
blocked-by: [material-one-binding-per-distinct-texture]
tags: [material, textures, memory, texture-packing]
---

# Repack into the canonical layout

## Why

Decisions D2, D4, D6, D7 (graphics § 39): whatever the source files, the engine builds at most three textures for
the core PBR maps — Albedo RGB + opacity A (sRGB), Normal RGB + height A (UNORM), Data R AO / G roughness /
B metalness / A free (UNORM). It also fixes a defect on the way: a separate opacity mask is RGBA with A = 255 today
and never reaches the coverage-preserving mips.

## What remains

- The repack (emeraude-base `Pixmap::fillChannel()` exists): from the material's components, wherever they come
  from (store JSON, glTF, FBX, USD, Toolkit foliage), build the canonical images; an asset already in the layout
  (glTF ORM, albedo with alpha) is used as is.
- D4: a channel without a source is 1.0 and the component's value moves into its UBO factor.
- Identity: a repacked texture is named by its recipe (sources, channel map, ratio) so two materials with the same
  recipe share one GPU texture; two with different opacity sources do not.
- Release: the repacked image is GENERATED — give it a recipe source so `reloadLocalDataFromOwnSource()` can rebuild
  it, else it stays resident; mark each source image releasable once the repack consumed it (an image no texture
  reads stays resident today).
- D7: the `TextureCache` key = hash of the final pixels + the package descriptor (layout, channel map, ratio, mip
  policy, encoder version), `TextureCache::Version` bumped.
- D6: measure the BC7 quality of normal + height in A (normal angular error, height error) before sealing the layout.
- Proof: VRAM and bindings against the reference, the same image at a pinned pose (per-pixel comparison).

## ⚠️ Traps

- The albedo is sRGB: only its A may carry linear data (opacity). Never pack linear data into its RGB.
- A cutout / blending decision reads the alpha source (`alphaSourceTextureComponent()`, `promoteBinaryCoverageToCutout()`):
  after the repack the alpha source is the albedo's A.

## References

- `docs/subsystems/graphics/39-material-texture-packing.md`, `30-compressed-image-path.md`,
  `31-alpha-coverage-a-mask-is-not-a-gradient.md`, `docs/subsystems/resources/03-resources-specific-rules.md`
  § CPU Copies, `alpha-mask-textures-lose-coverage-and-colour-in-the-mip-chain`.
