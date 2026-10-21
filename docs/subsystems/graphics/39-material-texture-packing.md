# Material texture packing and the GPU footprint policy (design, Oct 2026)

> Status: owner decisions D1-D13 of 2026-10-20; L0, L1, one binding per distinct texture, L2a and L2b DONE on Linux, work not started. The lots are the `docs/todo/` items listed at
> the end; this file keeps the design and the decisions, the items keep what remains.

## Goal

A lighter rendering: fewer textures bound per material, fewer fetches per pixel, less VRAM and bandwidth. The
GPU layout is decided by the engine **independently of the source files**: a loader hands whatever the asset
carries (separate maps, a glTF ORM image, an FBX opacity map…), the engine repacks it into a canonical layout and
then applies one footprint policy (resolution ratios per texture kind), the same for every loader.
**Load time is not a constraint** (owner): quality filters, per-kind mip chains and BC7 encoding at load are fine.

```
sources (any layout, any loader)
   │
   ▼  repack (engine) → canonical layout
   ▼  footprint policy: ratio per texture kind, per-kind mip chain
   ▼  BC7 (TextureCache, key = final pixels + package descriptor)
   ▼  upload, CPU copy released (recipe reload)
```

## The canonical layout

| Texture | RGB | A | Colour space |
|---|---|---|---|
| Albedo | albedo colour | opacity (coverage) | sRGB (Vulkan keeps A linear in an sRGB format) |
| Normal | normal XYZ | — (the normal stays ALONE, D6 revised) | UNORM |
| Data | R = ambient occlusion, G = roughness, B = metalness (glTF ORM order) | height (parallax / displacement) | UNORM |

Emission, specular colour and sheen colour stay separate textures (sRGB colour data cannot share a texture with
linear data). The other grey-scale components (clear coat, subsurface, transmission, thickness, iridescence…) stay
OUT of the canonical layout in this first pass; they only gain a working `SourceChannel`.

## Owner decisions (2026-10-20)

- **D1 — Generic mechanism + policy.** A material component reads `(texture, channel)`; the material binds ONE
  descriptor per DISTINCT texture (shared sampler, one fetch per texture). The canonical layout is a policy of the
  repack, not something Saphir knows: an asset already packed (glTF ORM) gains without any repack, and the layout
  can evolve without touching the code generation.
- **D2 — Data = glTF ORM order** (R AO, G roughness, B metalness), A left free for now.
- **D3 — Other grey-scale components** stay out of the layout; `SourceChannel` is honoured for every component.
- **D4 — A channel without a source** is filled with 1.0 (255). As implemented and accepted (owner, 2026-10-20): a
  component the definition gives as a VALUE stays a value component — no texture read at all, still changeable at
  run time — so nothing has to move into a UBO factor. AO 1.0 = no occlusion.
- **D5 — Ratios are global settings per texture kind** (albedo, normal, data), the same strategy for every loader;
  a per-material override may come later. **Implemented as skipped mip levels, OPTIONAL (owner, 2026-10-21)**: §
  GPU footprint per kind.
- **D6 — REVISED on the measurement (owner, 2026-10-20): the normal map stays alone, the height goes into the A of
  the Data texture.** Normal RGB + height A in one BC7 cost the normal its fine grain (relief, per pixel against the
  unpacked render: mean |d| 2.63, 5.6 % of the pixels off by more than 8, no bias); the height in Data A costs nothing
  measurable (1.15, the launch-to-launch noise being ~1) for the SAME footprint (Sand002: 3 textures, 16.0 MiB either
  way). The normal alone can become BC5 (D8).
- **D7 — The BC7 cache key = hash of the FINAL pixels (after repack and reduction) + a package descriptor** (layout,
  channel map, ratio / target size, mip policy, block format, encoder version and settings), with a
  `TextureCache::Version` bump.
  Keying on the recipe was rejected: it would still have to hash the decoded sources to detect a change.
  (2026-10-20: the mip levels asked are in the key now — a defect fix, caution-points § The BC7 texture cache.)
- **D8 — The block format follows the texture kind** (today every 2D texture is BC7):
  - Albedo (with or without opacity) and emission: **BC7 always** (sRGB) — quality first, the saving comes from the
    ratios (D5); BC1 was rejected.
  - **The normal map: BC5** (X, Y; Z reconstructed in the shader — a code-generation variant chosen by the format),
    same cost as BC7, much better normal precision. Since D6 was revised the normal is always alone.
  - Data (AO / roughness / metalness / height): BC7 (BC1 couples the channels too much for independent data).
  - **A lone grey-scale map** (the components outside the layout, D3): **BC4**, half the cost; the `ImageView`'s
    `VkComponentMapping` broadcasts R so the shader is unchanged. The CPU must keep one channel instead of forcing
    RGBA (`Abstract::validatePixmap()`).
  - The encoder: `rgbcx` (BC1/BC3/BC4/BC5), shipped in the same bc7enc_rdo library the engine already links.
  - **Sub-decisions (owner, 2026-10-20)**: the shaders reconstruct Z for EVERY normal map, whatever its format (one
    decode, no program variant); a normal map is known by an IDENTITY flag its creator sets (`Usage::normalMap`, the
    `#normal` variant — like `#sRGB` and `#flipY`), never by a content heuristic; BC4 is chosen by CONTENT (a linear
    texture whose every pixel is grey and opaque), never by a declared kind; the CPU copy kept on one channel is the
    NEXT step (this lot is the GPU side). Implemented: § Block format per kind.
- **D9 — The sources of a packed texture live in a GENERATED `ImageResource`** (owner, 2026-10-20), not in the
  texture classes: a "packed image" holds `std::array< ChannelSource, 4 >` (per output channel: the source image, its
  channel, a default value — D4), its sources are `ResourceTrait` dependencies, marked releasable once composed; it is
  rebuilt from that recipe when its CPU copy comes back (`reloadLocalDataFromOwnSource()`); the recipe's name is its
  identity (two materials with the same recipe share it). `Texture2D`, the BC7 cache and the mip chain stay as they
  are: a texture still uploads ONE image. Rejected: the four sources inside `Texture2D` and every texture class.
- **D10 — The fetch is shared only for the repack's canonical textures** (owner, 2026-10-20): one `texture()` per
  canonical texture, its components reading their channels from it; elsewhere one fetch per component (each
  component type keeps its own UV-transform slot). Details: § One binding per distinct texture.
- **L2 skeleton (owner, 2026-10-20).** The repack works on IMAGES, before any texture is requested (requesting a
  texture starts its load and upload at once): the recipe is one more SOURCE of `ImageResource` (`PackRecipe`, beside
  the loaders' `EncodedSource`): composed at load from its source images (each marked releasable once read), composed
  again when its CPU copy comes back, its name is its identity. `StandardResource` plans: it gathers the SOURCES of
  its canonical maps, builds the recipes, and only then requests the `Texture2D` (packed, or separate for what does not
  group). Order: L2a the recipe, L2b the store JSON materials (two-phase parse), L2c FBX and the Toolkit foliage,
  L2d glTF.
- **D11 — Only maps read at the same UVs group**: a map whose UV transform, UV set or normal flip differs stays a
  separate texture (an Info log once). Rejected: baking the transform into the pixels.
- **D12 — A recipe takes its LARGEST source resolution**, the others upscaled with a quality filter; the ratios (D5)
  reduce afterwards.
- **D13 — `Core/Graphics/Texture/Packing`**, true by default: the A/B switch and a diagnosis aid.
- **D14 — One shared packer** (owner, 2026-10-20): `Graphics/TexturePacker` takes a material's canonical maps as
  IMAGES (`PackSource`: image, role, channel, sampling key) and answers, per packed role, the packed texture and its
  channel. The store JSON planner, the FBX loader and the Toolkit foliage all use it — one grouping logic. Rejected:
  image-source setters inside the material.
- **Start**: measure first (per-texture VRAM accounting, reference scenes), then the lots in order.

## State of the engine before the work (survey, 2026-10-20)

- One lit material, `StandardResource`; every map is a `Component::Texture` with its own binding
  (`Component::Texture::create`, `Texture.cpp:264`), its own sampler name (`Saphir/Keys.hpp:695-734`) and its own
  fetch. The descriptor-set layout and the shared UBO bucket are keyed by the texture COUNT only
  (`StandardResource::getSharedUniformBufferIdentifier()`). A glTF ORM image is one resource bound three times and
  fetched three times.
- `SourceChannel` (`Component::Texture::sourceChannelSwizzle()`) is honoured by roughness, metalness, specular,
  clear coat, clear-coat roughness and sheen roughness only. AO, opacity, height, subsurface, transmission,
  iridescence and the reflectivity map read `.r` in hard; the two thickness maps read `.g`; the shadow pass reads
  `r` (opacity) / `a` (albedo) in hard; the RT alpha test reads opacity `.r`.
- CPU pixels are always RGBA8 (`Abstract::validatePixmap()`): a grey-scale map gets A = 255, so a separate opacity
  mask (Toolkit foliage `-alpha`, FBX opacity, store `*-alpha` maps) never reaches the coverage-preserving mips,
  which only look at the alpha channel (`AlphaCoverage`). Moving the mask into the albedo's A fixes that.
- Every 2D texture is BC7 on a BC-capable GPU (`TextureCompressor`, bc7enc_rdo): one byte per texel whatever the
  channel count — three grey-scale maps packed into one texture divide their footprint by three.
- `Texture2D::memoryOccupied()` answers `sizeof(*this)`: there was no per-texture VRAM accounting (only the VMA
  budgets) — since 2026-10-20 `Core.RendererService.getTextureMemory(top)` reports it (graphics § 37).
- RT (`GPURTMaterialData`, 7 vec4, the stride hard-coded in four GLSL files) carries six bindless indices; only
  roughness and metalness have a channel field.
- A repacked image is a GENERATED image: under the CPU-copy release rules
  (`docs/subsystems/resources/03-resources-specific-rules.md` § CPU Copies) it would stay resident, and its source
  images, read by the repack and not by a texture upload, too. The repack must carry a recipe that can rebuild the
  pixels and mark its sources releasable.
- projet-alpha's store: 3,779 of 3,933 materials carry one texture, about a hundred carry 2 to 5. The gain is in the
  scene loaders (glTF, FBX, USD) and those multi-map materials.

## One binding per distinct texture (2026-10-20, D1 part 1)

`StandardResource::createTextureComponents()` walks the texture components in `ComponentType` order (never the
`unordered_map`'s) and gives each DISTINCT texture (`Vulkan::TextureInterface`) one binding: the first component
leads it, the others call `Component::Texture::shareSamplerOf(leader)` — same binding, same sampler NAME (a static
`Uniform::*Sampler` literal: `Declaration::Sampler` keeps the pointer). `m_textureBindings`
(`StaticVector< TextureBinding, 27 >`, one entry at most per component type) drives the layout and the descriptor
writes, in binding order. `AbstractShader::declare(Sampler)` ignores an IDENTICAL re-declaration silently (same
set, binding, type, array size) and warns only on a real collision. The layout identifier adds the binding the mesh
stage reads (`…MeshBinding<n>`): keyed on the texture count alone, the first material to build a layout decided the
stage flags of every other material with that count.

Measured (Linux, validation on): `sponza` 207 → **162** bindings for the same 207 components (the 45 glTF ORM
materials: 4 components, 3 bindings), 0 `VUID-`, 0 sampler warning; the packed `Sand002` of the L1 proof: 5
components on **3** bindings (AO, height, roughness on one), the image within the launch-to-launch noise (mean |d|
0.89 vs the separate-binding capture; noise 0.99).

**Not done — the fetches.** Each component still calls `texture()` itself: one sampler, but as many fetches as
components. Sharing the fetch needs the components to read the SAME coordinates, and each component type has its own
UV-transform slot in the UBO (a VALUE, changeable at run time, never baked into a shared program). An owner rule is
needed first — **D10 (owner, 2026-10-20): the fetch is shared ONLY for the canonical textures the repack builds**
(one transform by construction); an asset already packed (a glTF ORM) keeps one fetch per component until the repack
takes it over. Rejected: a general "one transform per shared texture" rule (it would change the
KHR_texture_transform contract), and no fetch sharing at all.

## The repack (L2a + L2b, 2026-10-20)

- **L2a — `PackRecipe`** (`Graphics/ImageResource.hpp`): per output channel a source image and its channel, or a
  constant. `ImageResource::load(PackRecipe)` makes the sources dependencies and composes the pixels in
  `onDependenciesLoaded()` (outside the resource lock): each source leased (`acquireLocalData()`), the packed size the
  largest source's (D12), a smaller one upscaled by `Processor::resampleTexture()` (centres aligned, wrapping —
  NEVER `resize()`, below), the channels copied by `Pixmap::copyChannel()`, each source marked releasable once read.
  `releasesLocalData()` is true for a packed image: `reloadLocalDataFromOwnSource()` composes again.
- **L2b — the store JSON materials**: `StandardResource::load(json)` calls `planPackedTextures()` BEFORE parsing
  (a requested texture is loaded and uploaded at once). Two canonical textures: Albedo (albedo RGB + opacity →
  A, sRGB) and Data (AO → R, roughness → G, metalness → B, height → A). A group needs ≥ 2 of its maps as 2D store
  textures whose image exists, read at the same UVs (D11; else an Info log, the maps stay separate). The planner
  creates `Packed/<layout>/<source>.<swizzle>|…` (the image) and its texture (the usage suffixes `#sRGB` / `#flipY`),
  then REWRITES the definition: each grouped map reads that texture (`Name`) through its channel
  (`SourceChannel`); everything else of the definition (scales, thresholds, amounts) is untouched.
  `Component::Texture::findStoreTexture()` takes a `Packed/` name as is (the planner fixed its identity; the opacity
  reads the A of an sRGB albedo, linear in every format). `Core/Graphics/Texture/Packing` (D13, default true)
  switches the planner off.
- **RT parity for the opacity** (part of `rt-material-packed-channels`): `GPURTMaterialData::OpacityChannelShift`
  (flags bits 20-21), filled by `SceneMetaData`, read by the shared alpha test (`RTAlphaTestGLSL.hpp`,
  `RTOpacityChannelShift`). Without it the RT alpha test read the packed albedo's RED as the opacity: Grid003 came
  out +2.7 brighter (bias growing towards the camera); with the screen-space lane the packed and unpacked renders
  were already identical (0.71) — that is how it was attributed.

- **L2c — the setter-built materials** go through the same packer: the FBX loader (albedo, opacity, AO, roughness,
  metalness of a material; FBX maps carry no UV transform) and `Toolkit::vegetationMaterial()` (a foliage `-albedo` +
  `-alpha`); the opacity / AO / height setters take the channel, and `Component::Texture::hasExpectedColorSpace()`
  accepts either colour space for an ALPHA read. ⚠️ Not exercised by the current data: the store's only FBX (Paladin)
  has an albedo and a normal, and every vegetation of the store is a JSON material whose albedo carries its own alpha.

**Measured** (Linux, RTX 3070 Ti, `relief` with its ground replaced through `setGround()`, the camera exposure pinned,
per pixel against the same material with `Packing` off, launch-to-launch noise ~1.0–1.1):

| Material | Textures | Bindings | MiB | mean \|d\| | > 8 |
|---|---|---|---|---|---|
| `Grounds/Sand002` (colour, normal, AO, roughness, height) | 5 → 3 | 5 → 3 | 26.7 → 16.0 | 1.05 | 0.07 % |
| `Metals/Grid003` (colour 1024², opacity 512², normal, height) | 4 → 3 | 4 → 3 | 3.34 → 3.01 | 1.13 | 0.95 % |

`water-world`: 28 → 26 textures, 744.8 → 734.1 MiB, 26 → 24 bindings; `sponza` unchanged (glTF: L2d), 0 `VUID-`.

**The macOS peer (Apple M6, 2026-10-20)** confirmed every number (sponza 162 bindings, Sand002 packed, 0 VUID, MCP and
console conformance) and, with a noise floor of 0.21 there, measured the albedo + opacity packing of Grid003 at 0.66
(p99.9 8/255, all of it on the grate's slatted panels): a small, systematic change — the albedo RGB and the mask now
share BC7 blocks. Linux sees it at 1.42 against a ~1.1 noise. **Owner decision (2026-10-20): KEPT** — the normal cost
of an RGBA albedo in BC7 (a glTF albedo with alpha pays it natively), and the mask gains the coverage-preserving mips;
a finer BC7 for the textures with alpha is item `bc7-quality-for-textures-with-alpha`.

**The Windows peer (RTX 3500 Ada, MSVC 19.44, 2026-10-20)**: Release and Debug 0 warning (the `FillingType` and
`StaticVector` forward declarations included), base suite 2702 / 0 failed, sponza 162 bindings / 1899.3 MiB with the RT
lane, water-world 734.11 MiB / 24 bindings, Grid003 packed vs not 1.46–1.59 against a 0.69 noise with NO brightness bias
under RT (the opacity channel works), MCP conformance 1902 / 0. Console conformance counts depend on the loaded scene
(4935 water-world Linux, 4923 water-world macOS, 4899 relief Windows): compare MCP counts across machines.

**L2c + the cache fix (engine d35ca2f4), macOS peer, 2026-10-20**: AppleClang PARANOID 0 warning, `-Wdocumentation`
clean; water-world with the cache ON now serves full chains again (742.11 MiB, Sand002 16.0 MiB, every texture 12
mips; 733.44 MiB with the poisoned cache before), a settings copy asking one level gets one level (562.06 MiB), and a
default run right after still gets 12 — the mip count separates the entries. Grid003 still packed (0.635 against the
same unpacked render, as the previous lot); MCP 1902 / 0; no MoltenVK layout complaint. **Windows peer** (MSVC 19.44 Release +
Debug /W4 /WX 0 warning — the `StaticVector` pointer iterator and the `std::array` loop included): water-world 12 mips
everywhere, 734.11 MiB; a settings copy asking one level in the SAME scratch cache got one level and the cache grew
197 → 214 files (new keys, not served from the full chains); sponza 162 bindings with the RT lane; Grid003 packed,
equal to the previous lot within the noise; MCP 1902 / 0; 0 VUID beyond the known `setGround` 4+2.

**⚠️ Traps met on the way.**
- `Processor::resize()` is a DISPLAY resize (corners mapped, Black past the border): upscaling Grid003's 512² mask
  with it shifted the mask by half a texel and darkened every tile edge (15.3 % of the pixels off). The texture
  resample is `Processor::resampleTexture()` (emeraude-base).
- A packed opacity is invisible to RT until the RT data carries its channel (above). Any new packed role read by RT
  needs its channel field first.
- Six store materials name an albedo image that does not exist (`Metals/Grid001`, `Biologicals/Bones001`,
  `Liquids/WaterCaustic001` / `002`, `Miscellaneous/Clouds001`, `Miscellaneous/Flash001`): they render the default
  texture (random Perlin noise in Release, different at every launch) and are never packed. A data defect, not the
  engine's.

## One fetch per canonical texture (D10, 2026-10-20)

`StandardResource::declareSharedFetches()` declares, right after the parallax computed the coordinates, one
`const vec4 materialTexel<binding>` per binding that `sharesFetch()`: a PACKED canonical texture read by two components
or more at the base UVs (the height is excluded: the parallax marches it at its own UVs). It reads at the UVs of the
component leading the binding (D11 guarantees every reader the same values). `texelExpression()` answers that variable,
else the component's own `texture()`: the albedo, opacity, roughness, metalness and ambient-occlusion code goes through
it. Verified in the dumped GLSL: Grid003's albedo and opacity read `materialTexel1`; Sand002's roughness and AO read
`materialTexel3` at `pomTexCoords`; the images equal the previous lot within the noise (1.10 / 1.09).

**A/B (Ave Performus, Linux RTX 3070 Ti, `relief` + `setGround()`, pinned exposure, `ScenePass` GPU time, median of 5
interleaved launches each, validation off):**

| Material | Shared fetch | One fetch per component | Gain |
|---|---|---|---|
| `Metals/Grid003` (albedo + opacity: 2 → 1 fetch) | 1.085 ms (1.068–1.093) | 1.122 ms (1.113–1.131) | **+3.3 %**, outside the noise |
| `Grounds/Sand002` (AO + roughness: 2 → 1; the height stays in the parallax loop) | 1.598 ms (1.560–1.621) | 1.578 ms (1.542–1.622) | −1.3 %, inside the noise |

Below the plan's 5 % bar; **kept by the owner (2026-10-20)**: a small code, no regression measured, a real gain where the
shared reads dominate, and more to come with the glTF ORM (up to 3 fetches → 1, L2d).

**macOS peer** (M6, MoltenVK 1.4.2, the unshared side built from d4a67484, NOT interleaved — all unshared runs first):
`ScenePass` Grid003 6.519 → 6.249 ms (**+4.1 %**, the ranges do not overlap), Sand002 7.787 → 7.668 ms (+1.5 %, inside
the noise); the frame (~33 ms of post-processing there) −0.5 %. Images identical within the noise, the dumped GLSL as on
Linux (15 passes × 2 materials), no SPIRV-Cross / MSL complaint, 0 VUID beyond the known `setGround` ones.
**Windows peer** (RTX 3500 Ada, MSVC Release + Debug 0 warning, interleaved against d4a67484): images identical within
the noise (Grid003 1.33 vs 1.32–1.35, Sand002 1.25 vs 1.24–1.25); `ScenePass` median 0.998 → 0.968 ms (+3.0 %) but the
launch-to-launch spread there (0.8–1.1 ms) is larger than the effect: consistent with the gain, no regression.

## glTF (L2d, 2026-10-20)

The glTF loader hands the shared packer its data maps BEFORE resolving their textures: the metallic-roughness image's
G (roughness) and B (metalness) and the occlusion image's R. Only DECODED images (a KTX2 / Basis image is BC blocks
already: Sponza.ktx2 is never packed), with REPEAT addressing on both axes (the packed texture repeats), read at the
same UVs (the sampling key: the texture coordinate set and the `KHR_texture_transform` scale / offset / rotation, D11).
A packed metallic-roughness image is not created as a texture at all (no duplicate). The albedo's alpha is already in
place in glTF: nothing to pack there.

Measured (Linux, `asset-loader --demo-options 7,0,1,0,0,0`, DamagedHelmet, pinned exposure, validation on): its
`Material_MR` 5 → 4 textures, 5 → 4 bindings, 26.68 → 21.34 MiB, ambient occlusion / roughness / metalness through ONE
fetch (D10); the image equals the unpacked one (mean |d| 0.07 against a 0.05 noise); 0 VUID. Sponza unchanged (KTX2):
162 bindings, 0 VUID. **macOS peer** (M6): the same numbers (21.335 vs 26.669 MiB, 4 vs 5
bindings), 0 VUID; on/off 0.055 against a 0.026 noise — equal to the eye, measurable (the three data maps now share BC7
blocks, the same family as the albedo + opacity cost, much smaller); Sponza 162 bindings, never packed. **Windows
peer**: the same numbers, 0 VUID with the RT lane; inside the helmet 0.24–0.28 against a 0.08–0.11 noise, a fine speckle
following the texture detail, no shifted feature — the same BC7 shared-block cost (item
`bc7-quality-for-textures-with-alpha` now covers the packed data textures too).

## Animated textures in BC7 (2026-10-20)

`AnimatedTexture2D::createCompressedImage()`: on a BC-capable GPU every frame goes through the texture cache (BC7, its
own mip chain) and the frames are uploaded as the layers of one BC7 array (`Vulkan::Image::createFromMipChain()`
takes a layer count, the mips LAYER-major); RGBA8 stays the fallback (no BC, a frame that fails). Owner decisions:
BC7 now, BC5 for every normal map with D8; the ocean keeps its 120 × 1024² frames.

Measured (Linux, `water-world`, validation on, 0 `VUID-`): the ocean normal map 640.3 → **160.3 MiB** (120 layers, 11
mips); the scene's textures 734.1 → **242.2 MiB** (the explosion / fire / smoke sprites are animated textures too);
the first load compresses the 120 frames in ~40 s, a warm cache loads them in ~4 s. The water looks as before; a per-pixel
A/B is not possible (the normal map animates with time). **macOS peer** (M6, MoltenVK): the ocean 646.27 → 161.25 MiB, the scene
742.11 → 244.01 MiB, the sprites BC7_SRGB; ocean ready in 20.5 s cold, 2.95 s warm (the RGBA8 build: 2.78 s); 0 VUID, no
MoltenVK complaint on the BC7 array or its per-layer views; the same sea (open-sea patch mean ± σ within the animation
spread); MCP 1902 / 0. **Windows peer** (MSVC Release + Debug 0 warning, RTX 3500 Ada): 160.31 MiB, the scene 242.22
MiB; 49.8 s cold, 0.71 s warm; the RT lane (the per-layer 2D views in the bindless table) 0 VUID in water-world and
terrain; MCP 1902 / 0. `AnimatedTextureCubemap` keeps its uncompressed upload (no measured need yet).

## Block format per kind (D8, 2026-10-20)

`TextureCompressor::selectFormat(pixels, sRGB, normalMap)` decides at upload, in `Texture2D::createFromPixelData()` and
`AnimatedTexture2D::createCompressedImage()` (one format for every layer; BC4 only when every frame qualifies):

| The texture | Format | Cost / block |
|---|---|---|
| Declared a normal map (`Abstract::enableNormalMap()`), linear | **BC5** (X, Y) — rgbcx `encode_bc5_hq` | 16 B |
| Linear, every pixel grey and opaque (R = G = B, A = 255 — `isOpaqueGrey()`) | **BC4** — rgbcx `encode_bc4_hq` | 8 B |
| Everything else (colour, opacity, the packed Data texture, an sRGB grey) | BC7 (the fast preset as before) | 16 B |

- **The normal map identity**: `TextureResource::Usage::normalMap` → the `#normal` variant of a store texture
  (`getStoreTexture()`); the JSON material asks it for the `Normal` and `ClearCoatNormal` components (the `Texture`
  component's `normalMap` argument); the glTF, FBX and USD loaders cache THREE usages per image
  (`loaderUsageSlot()`: data, sRGB, normal map; suffixes `-data` / `-srgb` / `-normal`); `Toolkit::vegetationMaterial`
  asks it for `-normal`. Forgetting it is never wrong, only heavier: the map stays BC7 and renders the same. An sRGB
  normal map is a creator error: it stays BC7, with an error.
- **Z is reconstructed** in the three decodes (`StandardResource` normal and clear-coat normal, `RTR.cpp` hit
  shading): `xy = rg * 2 - 1`, `z = sqrt(max(1 - dot(xy, xy), 0))`, then `normalize(vec3(xy * scale, z))` (glTF's
  scale). A pre-encoded KTX2 BC5 normal map is right too (it used to read B = 0).
- **BC5 renormalizes every texel of every level before keeping X and Y** (`renormalizedNormals()`, on a copy: the chain
  keeps filtering the true box average). It is NOT optional: a source whose vectors are not unit (Pavement006: length
  1.071 on average, 0.98–1.20) reconstructs the wrong Z — 7.78° mean error without it, 0.77° with it.
- **BC4 is broadcast by the image VIEW**: `TextureCompressor::componentMapping(format)` maps a one-channel format
  (BC4, R8) to (R, R, R, 1), so every channel a reader samples (thickness reads G) is the grey it was in RGBA8 — the
  shaders are unchanged. Decided by the IMAGE format: a pre-encoded KTX2 BC4 gets it too.
- **The cache**: version 4, the format in the header and (BC5 / BC4 only) in the key — caution-points § The BC7 texture
  cache.
- **The library**: `rgbcx.cpp` and `rgbcx.h` are added to the bc7enc_rdo static library by ext-deps-generator's
  `patches/bc7enc_rdo.patch` (the upstream CMake builds them into tools only); a release of the ext-deps archives is
  needed for every OS.

Measured (Linux, RTX 3070 Ti, validation on, 0 new `VUID-` — the `relief` + `setGround` runs keep the 4 + 2 known ones of
`default-mesh-instance-destroyed-while-in-use`):

| Asset | Change | Image (per pixel, against the BC7 build; noise ~1.1) |
|---|---|---|
| `relief` + Sand002 | normal BC5 (5.3354 MiB, as BC7) | mean \|d\| 2.39, +0.44 brighter |
| `relief` + Grid003 | normal BC5; height BC4 0.335 → 0.169 MiB | mean \|d\| 3.67, 13.9 % > 8, a fine speckle on the rust, no feature moved |
| Grenade40mm height | BC4 1.335 → 0.669 MiB | — |
| `water-world` | the ocean BC5 (120 layers, 160.31 MiB, as BC7); the scene 242.2 → 241.55 MiB | the same sea (animated: no A/B) |
| DamagedHelmet (glTF) | normal BC5 (`4-normal`) | mean \|d\| 0.034 (noise 0.031) |
| `normal-map-debug` 4,2,0 (control) | both flip variants BC5 | top omni: the lit lip BELOW the groove on the authored quad (231.5 vs 210.9), above on the flipped one |

The image change is BC7's own error leaving. Offline round trip with the engine's encoders against the source (angular
error of the normal; |d| /255 of a height):

| Map | BC7 (engine preset) | BC5 / BC4 |
|---|---|---|
| Sand002 normal | 3.02° mean, p99 10.6°, max 37.6° | **0.92°**, p99 2.35°, max 13.7° |
| Grid003 normal | 4.48°, p99 25.1°, max 88.8° | **1.97°**, p99 9.1°, max 31.2° (unit source: the same renormalized) |
| Pavement006 normal | 1.52°, p99 7.6° | **0.77°**, p99 2.8° (renormalized; 7.78° without) |
| Grid003 height | 0.22, p99 2, max 6 | 0.61, p99 5, max 6 |
| Grenade40mm height | 0.21, p99 2, max 7 | 0.51, p99 4, max 12 |
| Sand002 height | 0.02, p99 1, max 4 | 0.15, p99 1, max 7 |

BC4 is a little less exact than BC7 on a grey map (it spends half the bytes on it), still under 2.5 % at the 99th
percentile; BC5 is two to three times more exact than BC7 on a normal map for the same bytes.

**Step 2 — a grey-scale image file keeps ONE channel on the CPU (2026-10-21, owner: files decoded as L only).**
`ImageResource::readLocalData()` keeps a `ChannelMode::Grayscale` pixmap as it is; every other mode (grey + alpha
included) is expanded to RGBA as before. Not `validatePixmap()` (the cubemap faces use it too); the scene loaders (glTF,
FBX, USD, WAD) decode RGBA themselves and are untouched. The CPU copy of such an image is a quarter of the RGBA one — the
store holds 79 of them (100 Mpx: 384 MiB as RGBA, 96 MiB as one channel); `land003-height` (the terrain, kept resident)
16 → 4 MiB. Every consumer reads it as the RGBA expansion did (R = G = B, A = 1):
- `TextureCompressor`: `compress()` / `compressSingle()` take one channel (each 4 × 4 block expanded, BC4 / BC7 alike; a
  BC5 request expands the image first, the renormalization needing three components); `isOpaqueGrey()` is true for it;
  the hole dilation runs on RGBA only.
- `PackRecipe::fillChannel()`: G / B of a one-channel source read its R, its A is 255.
- The views: `componentMapping()` broadcasts `R8_SRGB` too; `Texture1D` uses it like `Texture2D`. R8_SRGB is optional in
  Vulkan (`Texture2D::supportsGreyScaleColour()`): without it, the uncompressed fallback of `Texture2D` / `Texture1D`
  uploads the RGBA expansion, and an animated texture with grey colour frames is refused (the BC path takes them).
- `MovieResource` refuses frames of different channel counts (as it refused different sizes): the upload concatenates them.
- `CursorAtlas` expands to RGBA for GLFW.
- `ImageResource::isGrayScale()` (the channel mode) is now true for those files; no caller depends on it.
Measured (Linux): `relief` + Sand002 (its AO and height are L files) within noise (1.25, noise 1.3), the same formats;
`Doors/MetalDoor001` (an L sRGB albedo) BC7_SRGB, the image neutral (R, G, B 93.2 / 93.4 / 93.5), its height BC4;
`terrain` (the L heightmap) loads, 0 VUID. Not exercised: the R8_SRGB fallback (no BC-less device here).
**Validated on the three OS (2026-10-21):** macOS (M6, MoltenVK, paranoid 0 warning) and Windows (MSVC /W4 /WX Release +
Debug, the RT lane): Sand002 and Grid003 within noise, MetalDoor001 BC7_SRGB and neutral (macOS 44.2 / 47.1 / 51.7 at its
pinned exposure, Windows 87.0 / 87.5 / 88.2), the terrain heightmap loads, only the known `setGround` VUIDs. R8_SRGB in
practice (Windows, vulkaninfo): the Intel RaptorLake-S iGPU does not support it at all; the RTX 3500 Ada samples and
filters it but cannot BLIT into it — the uncompressed path builds its mips by GPU blit, so `supportsGreyScaleColour()`
rightly refuses on both and that path uploads the RGBA expansion (the BC path is taken anyway on every BC device).

⚠️ **Measurement trap met here**: a run on a COLD texture cache (the first after the format change) encodes on the
workers for tens of seconds — the ocean's 120 frames ~40 s — and a capture 6 s after the ground swap caught a scene
whose temporal state had not converged: 20 levels darker, the relief "stronger". The same capture 40 s later equals
the warm one (2.39 vs 2.33). Measure on a warm cache, or wait for the encodes to end.

**A reconstructed Z flattens FILTERED normals — accepted (owner, 2026-10-21).** Grid003 renders brighter on the three
OS (mean +2.02 Linux, +1.4 macOS, +2.15 Windows; Sand002 +0.43). Not BC5: the same build with the normal maps forced
BC7 and Z still reconstructed gives +2.44 (noise 0.00). The stored levels are right (each one renormalized); the
sampler's bilinear / trilinear / anisotropic filtering averages the X and Y of neighbouring texels into a SHORTER
vector, and `sqrt(1 - x² - y²)` then stands the normal up, where the former `normalize(xyz)` kept the mean direction:
a high-frequency map (the grating) loses a little slope contrast with distance. It is the behaviour of every
two-channel normal map (Godot, Unreal). A compensation would turn the lost length into roughness (Toksvig-style
specular anti-aliasing) — not opened.

**The BC5 encoder stays the HQ one (owner, 2026-10-21).** Offline, the engine's renormalized input: Sand002 BC7 3.02° /
BC5 HQ 0.92° / BC5 fast 1.09°; Grid003 4.48° / 1.97° / 2.34°; Pavement006 1.52° / 0.77° / 0.90°; the fast encoder is
~90× faster. HQ costs only the first, cold run (the cache serves afterwards): the ocean's 120 frames take 202 s cold on
Windows (49.8 s in BC7), 83.5 s on macOS (with the whole scene re-encoded after the cache version bump), 0.77 s / 3.09 s
warm.

**Validated on the three OS (2026-10-20/21, ext-deps v019):**
- **macOS** (M6, MoltenVK 1.4.2): cascade 0 warning (Apple clang paranoid), base 2701 / 0 failed, `-Wdocumentation`
  clean on the 10 touched TUs; Sand002 / Grid003 normals BC5, Grid003 height BC4 (0.167 MiB), only the known `setGround`
  VUIDs; the ocean BC5 161.25 MiB, 0 VUID; DamagedHelmet BC5, 0.075 (noise 0.055); `normal-map-debug` lit lip below /
  above as expected; no MoltenVK complaint on BC4 / BC5 or the (r, r, r, 1) view.
- **Windows** (RTX 3500 Ada, MSVC 19.44 /W4 /WX): Release and Debug 0 warning, no C4201 from `rgbcx.h` (an external
  include); base 2702 / 0 failed in both; the same formats and sizes; the ocean BC5 160.31 MiB with the RT lane, 0 VUID;
  DamagedHelmet 0.20 (noise 0.08–0.15); `normal-map-debug` as expected.
- **Linux glibc 2.43** (RTX 3050 Ti): cascade 0 warning, base 2702 / 0 failed, Sand002 normal BC5.

## Baked assets (option A, 2026-10-21)

Owner decision: the runtime pipeline can be run ONCE, offline, and its result shipped — a texture uploaded as stored, no
decode, no transcode, no encode. Three pieces:

- **`Graphics/KTX2Writer`** — writes a BC7 (sRGB / linear), BC5 or BC4 mip chain into a KTX2 container (libktx, the
  decoder's library), zstd-supercompressed (lossless), its `vkFormat` the real block format: `KTX2Decoder` passes it
  through, the image is uploaded as stored. The chain is checked against its format (sizes, halving dimensions).
- **`EmeraudeTextureBake`** (engine `tools/texture-bake/`, built on demand: `--target EmeraudeTextureBake`) — composes
  ONE texture of the canonical layout from image channels or constants with `PackRecipe::fillChannel()` (the runtime's
  own rule, extracted for it: the largest source size, a texture resample for the smaller ones), chooses the format with
  `TextureCompressor::selectFormat()` (`--kind colour|normal|data`), builds the chain with `TextureCompressor::compress()`
  and writes it. `--flip-normal-y` converts a Direct3D normal map.
- **`tools/gltf-bake-textures.py`** — rewrites a GLB from a manifest (per material and glTF slot: the recipe of its
  texture); identical recipes are baked once and stored once, in parallel; the BIN chunk is rebuilt (the old images
  dropped, the kept views — `EXT_meshopt_compression` ranges included — copied); the cache of baked containers is keyed
  on the recipe, the baker's hash and the zstd level. The textures stay behind `KHR_texture_basisu` (owner decision:
  the file is for Emeraude only — the extension requires Basis payloads; fastgltf refuses an unknown required
  extension, which ruled out a vendor one without patching it).

**Sponza** (projet-alpha.data `glTF/Sponza.baked.glb` + `Sponza.bake.json`, 2026-10-21): the 84 textures of
`Sponza.ktx2.glb` re-baked from the package's PNG sources at full resolution (31 materials, 89 slots); 1443 MB against
1192 MB for UASTC. Loaded by the engine with 0 VUID; the same VRAM (1886.8 MiB) and bindings (162); the normals now BC5
(27). Against the PNG sources (`arch_stone_wall_01`, level 0) the baked textures are 1.6 to 2.6 times closer than the
UASTC ones: albedo |d| 0.81 vs 2.15, normal 2.60° vs 5.63°, roughness / metalness 2.69 vs 4.31.
The bake found the colour defect of alpha textures (below, and engine item
`alpha-mask-textures-lose-coverage-and-colour-in-the-mip-chain`): the cypress leaves kept 56 % of their green at
mip 8; with the colour dilated, 117 → 119 (and their coverage 6.25–6.64 % from mip 0 to 10, where UASTC lost it at mip 10).

**The colour under transparent texels is dilated** (`Base::PixelFactory::Processor::fillTransparentColour()`, pull-push,
Gortler et al., *The Lumigraph*, SIGGRAPH 1996): `TextureCompressor::compress()` fills it before the BC7 chain — below the
alpha test's cutoff for a binary mask, at alpha 0 only for graded alpha; the alpha is never touched. `TextureCache`
version 5.

**Validated on the three OS (2026-10-21):** the tool, the writer, the dilation and `Sponza.baked.glb` (projet-alpha
`sponza --demo-options 0,120,1`, projet-alpha.data 20e18fb).
- **macOS** (M6, MoltenVK 1.4.2): cascade and the tool 0 warning (Apple clang paranoid), the template guard, base 2709 /
  0 failed; a baked Grid003 colour BC7_SRGB and normal BC5 (headers right, zstd); `relief` Grid003 against D8 0.28 (noise
  0.25), `normal-map-debug` identical; Sponza baked: 0 VUID, 1887.4 MiB / 162 bindings as option 0, 30 normals BC5, the
  cypress brighter and greener (G +3.5), elsewhere 1.27 without bias. Its load ended ~1.5 s LATER than UASTC (5.7 vs
  4.2 s to the last resource) — unexplained.
- **Windows** (RTX 3500 Ada, MSVC /W4 /WX): Release and Debug 0 warning, the tool too, base 2710 / 0 failed in both; the
  same baked headers; `relief` within noise; Sponza baked with the RT lane: 0 VUID, the BC5 normals in the bindless table,
  1886.8 MiB / 162 bindings, cypress G +6.0; loaded FASTER (10.1–10.8 s vs 11.3–11.4 s).
- **Linux** (RTX 3070 Ti): 3.47–3.95 s to the scene, 5.67–6.28 s to the last resource, against 3.86 / 6.27 s (one warm
  UASTC sample).

## GPU footprint per kind (D5, 2026-10-21)

Owner decisions: an OPTIONAL reduction for a machine short of VRAM — the default changes nothing; expressed as top mip
levels SKIPPED per kind (`Core/Graphics/Texture/AlbedoSkippedMips`, `NormalSkippedMips`, `DataSkippedMips`: 0 full
resolution, 1 half — the VRAM / 4 —, 2 a quarter — / 16 —; default 0); applied at LOAD only (the assets, the baked ones
included, stay full resolution: the reduction is the running machine's decision); MATERIAL textures only.

- **The kind** is the texture's identity (`Texture2D::skippedMipLevels()`): a normal map (`isNormalMap()`), a colour
  (`isSRGB()`: albedo, emission), the rest data (AO, roughness, metalness, height, masks).
- **Materials only, by exclusion** (owner): a texture that is not a material's asks `Usage::fullResolution` / 
  `Abstract::enableFullResolution()` (the `#full` variant of a store texture) and never skips — sprites
  (`SpriteResource`), the image viewer, light projections (projet-alpha's flashlight and spot cookies). With the
  defaults at 0, a consumer that forgets it is unaffected.
- **The pixel path** compresses (and caches) the FULL chain, then uploads from level k: changing a setting re-encodes
  nothing. The uncompressed path keeps the same chain from level k (§ The mip filter). **The KTX2 path** (UASTC
  or baked) starts its upload at level k — free, the chain ships ready-made. At least one level is always kept.
  `MaxDimension` (graphics § 30) still applies first to the KTX2 sources, whatever their kind.
- The reduced image is a level of the chain (renormalized for a normal map, coverage-preserved and colour-dilated for
  an alpha mask): no dedicated resampling filter — the chain's filter (§ The mip filter) serves the reduction and the
  distance alike.

Measured (Linux, RTX 3070 Ti, 3840 × 1990, validation on, 0 new `VUID-`; the study ran the same reduction through the
sampler's `minLod`, kind by kind):

| Case | VRAM | Image change (noise) |
|---|---|---|
| `relief` + Sand002, all kinds 1 | Sand002 16.0 → 4.0 MiB, scene 113.25 → 84.75 MiB | 2.99 mean, 5.85 in the near field (1.30 / 1.47) |
| `sponza` (UASTC or baked), all kinds 1 | 1886.8 → **528.3 MiB** | 0.86–0.92 (0.32) |
| study, `relief` near field, one kind at 1 | — | data 1.64 (= noise), albedo 3.1, normal 5.3 (noise 1.5) |
| study, `sponza`, one kind at 1 | — | data 0.40, normal 0.51, albedo 0.64 (0.32) |

Reading: data at 1 is invisible; on a 4096² asset seen in 4K (Sponza) every kind at 1 is barely measurable; near a
2048² ground the normals show it first. The flashlight (`#full`) and the smoke sprite keep their 2048² / 1024².

**Validated on the three OS (2026-10-21):** macOS (M6, MoltenVK, Apple clang paranoid 0 warning) and Windows (RTX 3500
Ada, MSVC /W4 /WX Release + Debug, the RT lane): the defaults change nothing (VRAM and image within noise); every kind at
1 gives Sand002 1024² 1.33 MiB, the `#full` flashlight and smoke sprite unchanged, Sponza 528.3–528.9 MiB in both
options with 0 VUID (the reduced KTX2 images in the RT bindless table too). The Sponza image change shrinks with the
screen: 0.86–0.92 at 3840 × 1990 (Linux), 0.25 / 0.04 at 2560 × 1440 (macOS), within the noise on Windows — the fewer
pixels, the less the dropped top level is sampled.

## The mip filter (2026-10-21)

Owner decisions: NVIDIA Texture Tools' **Kaiser** (not Lanczos-3, not Mitchell, not the box in linear); each level from
the **previous** one (NVTT's way; from the base cost ~12 × the first level of a 4K); an sRGB colour filtered in
**linear light**; the edges **mirrored** (NVTT's default — never mixes opposite edges: atlases, sprites, baked glTF
UVs); the **uncompressed path** rebuilt on the same chain in the same lot.

- **The filter** — emeraude-base `Processor::downsample(…, ReductionFilter::Kaiser, sRGB)` (its `ReductionKernel`:
  Kaiser-windowed sinc, half-width 3 destination pixels, alpha 4, stretch 1, each tap averaged over its source pixel,
  after I. Castaño's `src/nvimage/Filter.cpp`, public domain). The edge mirror repeats the edge pixel, as
  `VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT` does (NVTT skips it). The ringing of the negative lobes is clamped to the
  byte range. The alpha is always linear; an sRGB colour is decoded by a 256-entry table and encoded to the nearest
  byte. Base traps and numbers: its `docs/caution-points.md` § PixelFactory.
- **One chain for both paths** — `Graphics/MipChain::build(base, levels, treatment, visitor)` emits every level, base
  first: the colour under the holes dilated before any filter, each level the Kaiser of the previous one, a binary
  mask's coverage held on the emitted copy, a normal map renormalized on the emitted copy (the chain keeps the
  unnormalized, uncorrected level). `TextureCompressor::compress(source, levels, format, sRGB)` encodes each level;
  the uncompressed path of `Texture2D` and `AnimatedTexture2D` keeps them and uploads them as they are
  (`Vulkan::Image::createFromMipChain()`, renamed from `createFromCompressed()`: every level copied, nothing blitted,
  no `TRANSFER_SRC`, each level at a 4-byte offset in the staging buffer — a one-channel level may be any size).
  Before, that path blitted a 2 × 2 box on the GPU: no dilation, no coverage, no unit normals although the shaders
  reconstruct Z (item `alpha-mask-textures-lose-coverage-and-colour-in-the-mip-chain` point 3).
- **Grey by content, one channel** (owner, 2026-10-21, found by macOS-PA): an image grey and opaque by its CONTENT but
  stored in RGB(A) — a palette PNG (`Metals/Grid003-height`, mode P), a grey RGB file — is BC4 on the BC path
  (`selectFormat()` reads the content) and went up as RGBA8 on the uncompressed one (the file's channels, D8 step 2
  keeps one channel for L files only). The uncompressed path now extracts its red channel first
  (`TextureCompressor::isOpaqueGrey()`; an animated texture when EVERY frame is), a colour only where R8_SRGB is
  supported: `Grid003-height` R8_UNORM 512² 10 mips 0.337 MiB instead of 1.344, Grid003 12.0 → 11.0 MiB, the image
  1.91 from the BC path (Linux).
- **`Core/Graphics/Texture/BlockCompression`** (default true, owner decision): false takes the uncompressed path on a
  BC-capable device — an A/B of the encoders, the only way to exercise the path a device without BC takes. A KTX2
  source ships its blocks: unaffected.
- **`TextureCache` version 6**; the colour space is folded into the key of a LINEAR BC7 chain only (an sRGB BC7 keeps
  the version-3 key and replaces its file).
- **Cost** (CPU, a full 4096² RGBA chain, GCC -O3, best of 5): former box 178 ms with a 256 MiB `double`
  intermediate, Kaiser 259 ms, Kaiser sRGB 317 ms, the intermediate a ring of 13 rows. The BC encoders dominate.
- The baked assets follow the chain only when baked again: `Sponza.baked.glb` was re-baked with it (projet-alpha.data
  06d5b19, 1447.3 MB). At `sponza`'s start pose (option 2 = 1, 3840 × 1990, exposure pinned, three runs each): mean
  abs. change 2.86 against the box-chain asset (run-to-run noise 0.07–1.49), mean 151.6 → 152.6, the top half of the
  screen's spectrum +3 to +6 %, 0 `VUID-`. ⚠️ Three runs in nine started at ANOTHER camera pose (framing shifted, the
  image 43–56 apart): compare `sponza` captures only after checking the framing.

Measured (Linux, RTX 3070 Ti, 3840 × 1990, X11, validation on, `relief` with its ground replaced, the demo's camera,
exposure pinned f/8 1/250 ISO 100, a cold texture cache per run, HEAD built beside as the baseline; the only `VUID-`
are the known 4 × 00922 + 2 × 05137 of `setGround`, item `default-mesh-instance-destroyed-while-in-use`, in both builds):

| Case | Mean abs. change vs the box (run-to-run noise) | Power of the image, Kaiser / box, by screen frequency (fraction of the Nyquist: 0.03–0.125 / –0.25 / –0.5 / –0.75 / –1.0) |
|---|---|---|
| Sand002, far third | 2.11 (1.43) | 1.006 / 1.006 / 1.006 / 0.971 / **0.870** |
| Sand002, middle | 2.85 (1.51) | 1.009 / 1.017 / 1.015 / 0.986 / **0.829** |
| Sand002, near field (mip 0) | 1.97 (1.65) | 1.002 / 1.002 / 1.001 / 0.989 / 0.980 |
| Sand002, every kind at 1, near field (the reduced level) | 3.55 | 1.001 / 1.020 / 1.010 / **0.754 / 0.630** |
| Grid003 (alpha mask), far third | 1.84 | 0.963 / 0.992 / 0.968 / 0.952 / **0.879** |

Reading: what the screen can hold keeps slightly more contrast, and the top band — where a box-filtered level
shimmers (it lets 31 % of a detail above the new Nyquist fold back, the Kaiser 0.3 %) — loses 12 to 17 %. The near field
of a reduced texture (every kind at 1) loses its finest grain (−25 / −37 % in the top half): it is the magnified level's
own top octave, the Kaiser's transition band, where the box passed both detail and aliasing. Side by side at 1:1 the
two are hard to tell apart; no halo at the grid's hard edges. The means do not move (Sand002 ± 0.07, Grid003 + 0.3 far).
**The uncompressed path** (`BlockCompression` false) matches the compressed one within the encoders' error: mean
change 2.2–2.4 against a 1.4–1.65 noise, the BC7 / BC5 levels losing 3 % of contrast (std 16.5 → 15.96 far) — Sand002
64 MiB in RGBA8 against 16 MiB in BC7; the binary masks detected and held on both paths.

## Reference measurement (2026-10-20, before any packing)

Linux, RTX 3070 Ti (8 GB), Release, `Core.RendererService.getTextureMemory()` once the demo was built, validation
layer on (0 `VUID-`). MiB = the device memory of the loaded textures, each image counted once.

| Demo | Textures | MiB | Textured materials | Components = bindings | Formats |
|---|---|---|---|---|---|
| `sponza` (Sponza.ktx2, glTF) | 103 | 1899.3 | 64 | 207 | 95 × BC7 4096² (21.3 MiB each, 13 mips), sky cube RGBA16F 64 MiB |
| `water-world` | 28 | 744.8 | 17 | 26 | ocean normal map 640.3 MiB (below), BC7 2048² store maps 5.3 MiB each |
| `jungle-ruins` | 76 | 379.2 | 32 | 74 | NOT a reference yet: only the Pyramid sub-asset gets its textures (USD texture paths, `usd-texture-paths-relative-to-authoring-layer`); its build took 560 s under the validation layer |

- **Sponza**: 45 materials bind one glTF ORM image twice (roughness `.g`, metalness `.b`) — 4 components, 3 distinct
  textures, 4 bindings each; no ambient-occlusion component is created. The 1:1 component/binding ratio is what
  one-binding-per-distinct-texture lot lowered (207 → 162 bindings with nothing else changed).
- **Sand002** (`water-world`): 5 maps × 5.3354 MiB = 26.7 MiB, 5 bindings; the canonical layout makes it 3 textures
  (albedo, normal, data AO + roughness + height) = 16.0 MiB, 3 bindings, before any ratio — measured so since L2b.
- **The ocean normal map** (`BuiltinOceanNormalMap`, `AnimatedTexture2D`): 120 layers × 1024², 11 mips, RGBA8
  UNORM UNCOMPRESSED = 640.3 MiB, 86 % of the scene's textures (BC7 since 2026-10-20: § Animated textures in BC7).

## The lots (`docs/todo/`)

1. ~~Measure first~~ — DONE, validated on the 3 OS 2026-10-20 (`getTextureMemory`; `jungle-ruins` to measure once its
   USD textures load).
2. ~~SourceChannel ignored by most components~~ — DONE, validated on the 3 OS 2026-10-20 (graphics § 08-8).
3. ~~One binding per distinct texture~~ — DONE, validated on the 3 OS 2026-10-20 (§ One binding per distinct texture).
4. `material-texture-canonical-repack` — D2, D4, D6, D7, recipe release.
5. ~~`texture-footprint-ratios-per-kind`~~ — D5 DONE 2026-10-21 (§ GPU footprint per kind); ~~`mip-chain-quality-filter`~~
   DONE 2026-10-21 (§ The mip filter; Linux measured, the peers' validation pending).
6. `rt-material-packed-channels` — the RT parity.
7. ~~`texture-block-format-per-kind`~~ — D8 DONE: the GPU side 2026-10-20, the one-channel CPU copy 2026-10-21
   (§ Block format per kind).
