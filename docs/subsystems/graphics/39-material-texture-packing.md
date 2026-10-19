# Material texture packing and the GPU footprint policy (design, Oct 2026)

> Status: DESIGN — owner decisions D1-D8 of 2026-10-20, work not started. The lots are the `docs/todo/` items listed at
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
| Normal | normal XYZ | height (parallax / displacement) | UNORM |
| Data | R = ambient occlusion, G = roughness, B = metalness (glTF ORM order) | free (unused for now) | UNORM |

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
- **D4 — A channel without a source** is filled with 1.0 (255) and the component's value goes into its UBO factor
  (the shader already computes `texture × factor`), so it stays changeable at run time. AO 1.0 = no occlusion.
- **D5 — Ratios are global settings per texture kind** (albedo, normal, data), the same strategy for every loader;
  a per-material override may come later.
- **D6 — Normal = RGB normal + A height.** BC7 modes 4/5 encode A apart from RGB; the quality is measured before the
  layout is sealed.
- **D7 — The BC7 cache key = hash of the FINAL pixels (after repack and reduction) + a package descriptor** (layout,
  channel map, ratio / target size, mip policy, block format, encoder version and settings), with a
  `TextureCache::Version` bump.
  Keying on the recipe was rejected: it would still have to hash the decoded sources to detect a change.
- **D8 — The block format follows the texture kind** (today every 2D texture is BC7):
  - Albedo (with or without opacity) and emission: **BC7 always** (sRGB) — quality first, the saving comes from the
    ratios (D5); BC1 was rejected.
  - Normal + height: BC7 (four channels). **Normal WITHOUT height: BC5** (X, Y; Z reconstructed in the shader — a
    code-generation variant chosen by the format), same cost as BC7, much better normal precision.
  - Data ORM: BC7 (BC1 couples the channels too much for independent data).
  - **A lone grey-scale map** (the components outside the layout, D3): **BC4**, half the cost; the `ImageView`'s
    `VkComponentMapping` broadcasts R so the shader is unchanged. The CPU must keep one channel instead of forcing
    RGBA (`Abstract::validatePixmap()`).
  - The encoder: `rgbcx` (BC1/BC3/BC4/BC5), shipped in the same bc7enc_rdo library the engine already links.
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
  `material-one-binding-per-distinct-texture` lowers (207 → 162 bindings with nothing else changed).
- **Sand002** (`water-world`): 5 maps × 5.3354 MiB = 26.7 MiB, 5 bindings; the canonical layout makes it 3 textures
  (albedo, normal + height, data AO + roughness) = 16.0 MiB, 3 bindings, before any ratio.
- **The ocean normal map** (`BuiltinOceanNormalMap`, `AnimatedTexture2D`): 120 layers × 1024², 11 mips, RGBA8
  UNORM UNCOMPRESSED = 640.3 MiB, 86 % of the scene's textures (item `animated-texture-frames-uncompressed`).

## The lots (`docs/todo/`)

1. `texture-vram-accounting-per-texture` — measure first.
2. `material-source-channel-ignored-by-hard-coded-components` — the defect.
3. `material-one-binding-per-distinct-texture` — D1.
4. `material-texture-canonical-repack` — D2, D4, D6, D7, recipe release.
5. `texture-footprint-ratios-per-kind` — D5, per-kind mip chains.
6. `rt-material-packed-channels` — the RT parity.
7. `texture-block-format-per-kind` — D8.
