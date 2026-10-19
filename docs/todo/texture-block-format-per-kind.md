---
id: texture-block-format-per-kind
title: Choose the block format per texture kind — BC5 for a normal without height, BC4 for a lone grey-scale map
status: open
priority: unranked
scope: Graphics/TextureResource, TextureCompressor, TextureCache, Vulkan/ImageView, Saphir (normal Z)
opened: 2026-10-20
blocked-by: [material-texture-canonical-repack]
tags: [textures, memory, compression, texture-packing]
---

# The block format per texture kind

## Why

Decision D8 (graphics § 39): every 2D texture is BC7 today (`Texture2D::createFromPixelData()`), one byte per texel
whatever its channels. Albedo, emission, Data ORM and normal + height stay BC7; a normal WITHOUT height becomes BC5
(better precision, same cost) and a lone grey-scale map BC4 (half the cost).

## What remains

- Integrate `rgbcx` (BC4 / BC5) from the bc7enc_rdo library already linked; check that its header ships in the
  ext-deps archive (ext-deps-generator otherwise).
- `TextureCompressor`: encode BC4 / BC5 with the per-kind mip chain; the block format enters the `TextureCache`
  package descriptor (D7).
- BC4: keep ONE channel on the CPU (`Abstract::validatePixmap()` forces RGBA today — its TODO) and broadcast R with
  the `ImageView`'s `VkComponentMapping` (identity everywhere today) so the shader is unchanged.
- BC5 normal: the code generation reconstructs Z (`sqrt(1 - x² - y²)`) when the normal texture is two-channel; RT
  shaders reading the normal (`RTR.cpp`) too (`rt-material-packed-channels`).
- The RGBA8 fallback of a GPU without BC keeps today's behaviour.
- Pre-encoded KTX2 BC4 / BC5 pass through unpaired today (`KTX2Decoder.cpp` sRGB pair table): they are linear only,
  which is right for these kinds.
- Proof: VRAM against the reference, normal angular error BC5 vs BC7, the image at a pinned pose.

## References

- `docs/subsystems/graphics/39-material-texture-packing.md`, `30-compressed-image-path.md`,
  `imposter-atlas-block-compression` (proposes BC5 for the imposter normals: share the encoder).
