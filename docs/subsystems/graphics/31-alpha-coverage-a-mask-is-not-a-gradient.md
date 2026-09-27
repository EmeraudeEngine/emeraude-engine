## Alpha COVERAGE: a mask is not a gradient (Sep 2026)

`Graphics/AlphaCoverage.hpp` — `coverage()`, `isBinaryMask()`, `rescaleToCoverage()`.

A box filter preserves the alpha MEAN; an alpha test reads the alpha COVERAGE. The two diverge
without bound as a binary mask is minified. Measured on Sponza's cypress mask (4096², **99.86 %
binary**): the fraction passing a 0.5 cutoff falls 6.34 % → 1.56 % (mip 9) → 0 % (mip 10), while the
fraction passing the G-buffer's 0.01 blend floor RISES 6.41 % → 26.56 % → **100 %** (mip 11). Far
foliage therefore either vanishes or stamps depth and the whole G-buffer over its entire QUAD.

Three pieces, all decided **on the pixels, never on the declaration**:

1. **Coverage-preserving mips** (Castaño, NVIDIA 2010) in `TextureCompressor::compress()`, gated on
   `isBinaryMask()` so graded alpha is untouched. ⚠️ The solver returns the bisection bound whose
   coverage is CLOSEST to the target — coverage is a step function, and the upper bound on a 1×1
   level means "the whole quad is opaque". ⚠️ The chain keeps filtering the UNCORRECTED level, or
   the gain compounds and the mask goes opaque.
2. **`TextureResource::Abstract::isBinaryAlphaMask()`** — virtual, defaults to `false`, overridden
   by `Texture2D`. A block-compressed source answers `false` (it would have to be decoded).
3. **`Material::Interface::onBeforeCreation()`** — ⚠️⚠️ the ONLY window where a material may still
   change its FLAGS: creation bakes them into the descriptor set layout and the program-cache key,
   and the loaders returned long before. `StandardResource::promoteBinaryCoverageToCutout()` runs
   there and turns a mis-declared `alphaMode = BLEND` carrying a binary mask into a real cutout.
   ⚠️⚠️ Derived classes override THIS, never `onDependenciesLoaded()` — that one is private because
   it IS the GPU creation, and an override of it that forgot to chain left 133 materials uncreated
   and the window BLACK.

⚠️⚠️ **None of it reaches a KTX2 asset.** `Sponza.ktx2.glb` keeps 84 of 84 images block-compressed
(UASTC transcoded block-to-block, never decoded), so the mask is never pixels. This path is
**unverified end to end at runtime** — no shipped scene exercises it yet.

⚠️⚠️ **A FIXED threshold loses a SPARSE mask at distance, and Castaño cannot save it** (forest, 2026-09-23).
The conifer needle card (`leaf007-alpha`, a red-channel JPEG mask) is **23 %** opaque. Box-filtered, its
tiny mips average to 0.23 — under 0.5, the whole card discarded, and the far pines rendered as bare trunks
even with every tree forced to LOD 0 (the aspen's leaf, 51 %, rounds UP and survived). Castaño's correction
does not help there: on a 2×1 or 1×1 level a pixel can only be all or nothing, and the nearest bound of a
23 % target is **0** — the same vanishing, deliberately (see piece 1). The fix is the **hashed cutout**
(§ Alpha Test): it wants the alpha MEAN, which is exactly what the box filter keeps, and keeps 23 % of the
pixels at every distance. A red-channel mask never goes through Castaño (`isBinaryMask()` reads the ALPHA
channel), which is what a hashed mask needs; a patch extending Castaño to red masks was written and
reverted for that reason. Measured at the owner's pose (eye (0, 19.2, 120)): bare trunks → full crowns,
0 VUID. Cost, wind frozen, 8 frames covering the jitter cycle: the broadleaf crop's >8/255 peak-to-peak
goes 0.35 % → 0.60 % (the per-pixel threshold under the sub-pixel TAA jitter), the pine crop 1.33 % with
crowns against 0.55 % bare.

⚠️ `TextureCache::Version` must be bumped whenever the BLOCKS a given pixmap compresses to change,
not only when the file layout does: the key hashes the SOURCE pixels, so an older blob stays a valid
hit forever otherwise.
