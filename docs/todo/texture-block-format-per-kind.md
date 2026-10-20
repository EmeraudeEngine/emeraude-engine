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

Decision D8 (graphics § 39): every 2D texture was BC7 (`Texture2D::createFromPixelData()`), one byte per texel
whatever its channels. Albedo, emission and Data (AO / roughness / metalness / height) stay BC7; the normal map — alone
since D6 was revised — becomes BC5 (better precision, same cost) and a lone grey-scale map BC4 (half the cost).

## Done (2026-10-20/21 — the GPU side, validated on the three OS with ext-deps v019)

BC5 for a declared normal map, BC4 for an opaque grey linear texture, the view swizzle, Z reconstructed in every
decode, the cache version 4, rgbcx in the ext-deps library: graphics § 39 § Block format per kind (decisions,
mechanism, measurements). Owner sub-decisions recorded under D8.

## What remains

- **The CPU copy on one channel** (owner: the next step): `Abstract::validatePixmap()` forces RGBA (its TODO); a grey
  map could stay one channel through the mip chain and the encoder (`encode_bc4_hq` takes a stride of 1), halving its
  CPU copy — and an uncompressed fallback in R8 already gets the broadcast swizzle.

## ⚠️ Traps

- BC5 needs unit vectors: the renormalization of every level is part of the encoding (7.78° → 0.77° on Pavement006).
- Measure on a warm texture cache: a cold run encodes for tens of seconds and an early capture is not converged.

## References

- `docs/subsystems/graphics/39-material-texture-packing.md` § Block format per kind, `30-compressed-image-path.md`,
  `imposter-atlas-block-compression` (proposes BC5 for the imposter normals: the encoder is in place).
