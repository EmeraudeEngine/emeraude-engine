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
  a per-material override may come later.
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
own mip chain) and the frames are uploaded as the layers of one BC7 array (`Vulkan::Image::createFromCompressed()`
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

⚠️ **Measurement trap met here**: a run on a COLD texture cache (the first after the format change) encodes on the
workers for tens of seconds — the ocean's 120 frames ~40 s — and a capture 6 s after the ground swap caught a scene
whose temporal state had not converged: 20 levels darker, the relief "stronger". The same capture 40 s later equals
the warm one (2.39 vs 2.33). Measure on a warm cache, or wait for the encodes to end.

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
5. `texture-footprint-ratios-per-kind` — D5, per-kind mip chains.
6. `rt-material-packed-channels` — the RT parity.
7. `texture-block-format-per-kind` — D8: the GPU side done 2026-10-20 (§ Block format per kind); the one-channel CPU
   copy remains.
