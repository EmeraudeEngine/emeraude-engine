---
id: alpha-mask-textures-lose-coverage-and-colour-in-the-mip-chain
title: An alpha mask loses its COVERAGE and half its COLOUR down a box-filtered mip chain
status: in-progress
priority: high
scope: Graphics/TextureCompressor, Graphics/TextureResource, Vulkan/ImageTransferOperation
opened: 2026-09-15
tags: [texture, foliage, measured]
---

# An alpha mask loses its COVERAGE and half its COLOUR down a box-filtered mip chain

## Why

A box filter preserves the alpha MEAN. An alpha test reads the alpha COVERAGE. The two diverge
without bound as a binary mask is minified, and the engine's mip chain is a plain box filter
(`vkCmdBlitImage` with `VK_FILTER_LINEAR`, or an area-weighted box filter, `Processor::downsample()`, on the BC7
path — it was a bilinear `Processor::resize` until `TextureCache::Version` 3).

Measured on Sponza's cypress mask (4096², 93.59 % of texels at alpha ~0 and 6.27 % at ~1, i.e.
**99.86 % binary** — it is a CUTOUT declared `alphaMode = BLEND`):

| mip | passes the old 0.01 G-buffer floor | real coverage (`a ≥ 0.5`) | factor |
|---|---|---|---|
| 0 | 6.41 % | 6.34 % | 1.0× |
| 6 | 14.23 % | 6.32 % | 2.3× |
| 9 | 26.56 % | 1.56 % | **17×** |
| 11 | **100 %** | 0 % | ∞ |

Two opposite failures from the same cause: a plain cutout makes distant foliage **thin out and
vanish**, while the engine's 0.01 blend floor made it **stamp depth and the whole G-buffer over its
entire quad**.

## And the colour goes with it

Owner's hypothesis — *"elle prend peut-être trop dans le transparent"* — confirmed, with the sign
inverted. The texture carries **no alpha dilation**: the RGB under the transparent texels is pure
black.

| zone | texels | mean RGB |
|---|---|---|
| opaque (a ≥ 250) | 1 044 349 | (117.3, **143.6**, 58.7) |
| edge (10 < a < 245) | 26 516 | (60.0, 68.5, 44.0) — **48 % of the true colour** |
| transparent (a ≤ 5) | 15 701 731 | **(0, 0, 0)** |

So the black bleeds into every filtered texel. Sampled where the mask still passes the cutoff:

| mip | green | vs true |
|---|---|---|
| 1 | 139.0 | 97 % |
| 5 | 107.5 | 75 % |
| 9 | 73.5 | **51 %** |

The leaf loses half its colour with distance while the reflection it receives does not — the
reflection's relative weight **doubles on its own**. Up close the same defect darkens the edge
texels to 48 %, which is what turns a legitimate specular glint into a hard white rim against a
near-black border.

## What is delivered

- **Coverage-preserving mip generation** (Ignacio Castaño, *Computing Alpha Mipmaps*, NVIDIA 2010),
  in `TextureCompressor::compress()`, gated on `AlphaCoverage::isBinaryMask()` so graded alpha (a
  glass pane, a decal at a uniform opacity) is left strictly alone. Verified offline on the cypress
  mask: coverage held at 6.25–6.35 % from mip 1 to mip 10, where the box filter reads 0 %.
- **`Graphics/AlphaCoverage.hpp`** — `coverage()`, `isBinaryMask()`, `rescaleToCoverage()`.
- **`TextureResource::Abstract::isBinaryAlphaMask()`** — a virtual defaulting to `false`, overridden
  by `Texture2D`, which delegates to `ImageResource::isBinaryAlphaMask()` (answered after the upload
  drops the CPU copy, engine `939b3fc2e`).
- **`Material::Interface::onBeforeCreation()`** — a new hook, the only window where a material may
  still change its FLAGS (creation bakes them into the descriptor set layout and the program-cache
  key, and the loaders have long returned). `StandardResource::promoteBinaryCoverageToCutout()`
  runs there and promotes a mis-declared BLEND to a real cutout **on the pixels, never on the
  declaration**.
- The `0.01` blend floor in `SceneRendering.cpp` now gates on the `BlendingEnabled` FLAG, so a
  promoted cutout leaves that branch.
- `TextureCache::Version` bumped to 2 — the key hashes the SOURCE pixels, so an older blob would
  stay a valid hit forever otherwise. It is 3 today (`TextureCache.hpp:165`: every level an exact
  area-weighted box filter).

## What remains

1. ⚠️⚠️ **None of it reaches Sponza.** `Sponza.ktx2.glb` logs *"84 of 84 image(s) kept
   block-compressed from the KTX2 payload (no decode, no CPU compression pass)"*: the leaf mask is
   UASTC transcoded block-to-block and never becomes pixels, so neither the measurement nor the
   correction can touch it. It is verified by construction and **unverified end to end at runtime** —
   nothing in that scene exercises it. Decide whether a block-compressed alpha source should be
   decoded once at load so it can be measured. Still so on 2026-10-13 (the log line is
   `src/Scenes/Loaders/GLTFLoader.cpp:2493`).
2. ~~**Alpha dilation (edge padding)**~~ — DONE 2026-10-21 (owner: dilation): base
   `Processor::fillTransparentColour()` (pull-push, 4 unit tests), called by `TextureCompressor::compress()` before the
   BC7 chain (below the cutoff for a binary mask, at alpha 0 for graded alpha), `TextureCache` version 5. Measured on the
   cypress leaves baked from their PNG (graphics § 39 § Baked assets): the green of the visible texels 143 at mip 0,
   141 at mip 8 (137 → 80 before; the UASTC asset itself 118). It cannot recover the author's already-darkened edge texels.
   Point 1 is now exercised end to end: `Sponza.baked.glb` carries the cypress mask through the engine's own chain.
3. ~~The **GPU blit fallback**~~ — DONE 2026-10-21 for `Texture2D` and `AnimatedTexture2D`: their uncompressed path
   builds the SAME chain on the CPU (`Graphics/MipChain::build()`: dilation, held coverage, Kaiser, graphics § 39
   § The mip filter) and uploads it without a blit; `Core/Graphics/Texture/BlockCompression` false exercises it (Grid003:
   the binary masks detected and held on both paths). Other users of the blit (`Texture1D`, cubemaps, 3D textures)
   still blit.

## ⚠️ Traps

- ⚠️⚠️ **The cutout promotion is NOT the near-field defect.** Up close the mask is sampled near
  mip 0, the coverage is already correct, the silhouettes are crisp — and the canopy still washes
  out. That half is the grazing-normal Fresnel spike:
  `docs/subsystems/graphics/32-the-reflection-effects-owed-the-environment-brdf-not-a-fresn.md`.
  Do not credit this work with that symptom.
- ⚠️⚠️ **The measured histogram came from a PNG in a DIFFERENT package** (`pkg_c_trees`), not from
  the KTX2 actually loaded. Same dimensions and same source art, but that identity was never proven.
- ⚠️ **The bisection must return the bound whose coverage is CLOSEST to the target**, never the last
  probe and never the upper bound on principle. Coverage is a monotonic STEP function of the scale:
  a 4×4 level can only express multiples of 1/16 and a 1×1 level only 0 or 1, so the upper bound
  there means "the whole quad is opaque" — the very defect being removed, reintroduced at the tail
  (measured: 12.5 % at mip 10 and 100 % at mip 12, against a 6.34 % target).
- ⚠️ **The chain must keep filtering the UNCORRECTED level.** Feeding the correction back into the
  next downsample compounds the gain and turns the mask opaque a few levels down.
- ⚠️⚠️ **`Material::Interface::onDependenciesLoaded()` is private because it IS the material's GPU
  creation.** Overriding it without chaining left every material uncreated — *"The PBR material
  '...' is not created ! It can't configure the light generator."* on 133 materials, 14 986
  renderables failing, and a BLACK window. That is why `onBeforeCreation()` exists.
- ⚠️ Disabling mipmapping globally (`Core/Graphics/Texture/MipMappingLevels = 1`) is a decisive
  DIAGNOSTIC but never a measurement: it also sharpens every normal map, and the stone control moved
  27.57 → 18.89 with its saturation going 12 % → 20 %. Read the image, not the numbers.
