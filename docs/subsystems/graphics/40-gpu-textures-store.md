# The `GPUTextures` store — GPU-ready material textures (design, Oct 2026)

> Status: L1 to L6 implemented (2026-10-21/22, § Implementation), validated on the three OS (2026-10-22). Owner
> decisions 2026-10-21 (T1–T15 below; T9 revised 2026-10-22). Open work: the engine items listed in § Open work. Builds on graphics § 39 (packing, block formats, the mip
> filter, the baked assets) and § 30 (the compressed image path).

## Why

The `Images` store mixes two contracts: **authoring sources** (PNG, JPEG — widely usable, decoded, repacked,
block-compressed and cached by the engine at load) and **runtime artefacts** (KTX2 already packed, block-compressed,
with their mip chain — uploaded as they are). The mix had a dead end: a `.ktx2` in `Images` is registered by the
`CompressedImageResource` container, yet a store texture named by a material always loads through `ImageResource`
(`Texture2D::load(filepath)`), so only the glTF loader could use a baked texture (graphics § 39 § Baked assets).

Goal (owner): the engine serves EVERY delivery — a finished product released with GPU-ready textures and no texture
compute at load, a development build from the sources with the cache at work, and any mix of the two.

## Owner decisions (2026-10-21)

- **T1 — Two stores, two contracts.** `Images` holds classic, widely usable images only (PNG, JPEG, …). A new store,
  **`GPUTextures`** (the name says what it holds, owner), holds the formats the PBR uses directly: packed,
  block-compressed, with their mip chain.
- **T2 — Naming convention, strictly checked.** A `GPUTextures` file is `<TextureName>.<Purpose>.<extension>`. The
  Purpose strictly describes the component(s) the file feeds; a badly named file is REJECTED (never guessed).
- **T3 — The Purpose lists the channels explicitly**, in channel order, with tokens from a strict vocabulary: one token
  per component, joined by `-`. A one-token file is a single-component texture. The file describes itself, and keeps
  its meaning if the canonical layout evolves.
- **T4 — `<TextureName>` is the material's name**, overridable: a material `Grounds/Sand002` looks for
  `GPUTextures/Grounds/Sand002.<Purpose>.<extension>`, with nothing to declare, in JSON as in C++; an optional key
  (working name `TextureSet`) lets several materials share one set (tint or tiling variants).
- **T5 — Resolution order: `GPUTextures` first, then `Images`.** A component found in `GPUTextures` is uploaded as
  stored; otherwise it falls back on the image the material declares, through the pixel path (repack, block
  compression, the `TextureCache`).
- **T6 — The engine ships the tools** that take sources (mostly from `Images`) and produce the `GPUTextures` entries;
  the `TextureCache` stays, for development and for the particular cases (procedural images, a missing entry, a device
  without BC).
- **T7 — Every container is accepted, think of the future**: the store reads through a container-agnostic decoder —
  KTX2 with raw BC blocks first, KTX2 with Basis Universal (UASTC / ETC1S, transcoded at load), and any later container
  (DDS, …) as a new decoder, never a new store.
- **T8 — Every `.ktx2` moves to `GPUTextures`.** `Images` accepts the classic formats only; a GPU container left there
  is refused with a message pointing to `GPUTextures`. projet-alpha is a testbed: what breaks there is moved, not
  kept compatible. (On 2026-10-21 `Images` holds no `.ktx2`.)
- **T9 — glTF: only a glTF with SEPARATE files (`.gltf`)** also looks its materials up in `GPUTextures` by material
  name; a `.glb` keeps its embedded images (§ 39 § Baked assets).
  **REVISED 2026-10-22 (owner, L6): a glTF never looks in `GPUTextures`.** A `.gltf` is made GPU-ready by baking its
  textures INTO the asset: `tools/gltf-bake-textures.py` writes them as KTX2 files beside it and points its images to
  them (`KHR_texture_basisu`), which the loader already reads. The asset stays self-contained, no set-name rule is
  invented for glTF materials, the loader is unchanged.
- **T10 — A material built in C++ from separate sources gets the repack as a HIDDEN step**, as a JSON, glTF or FBX
  material does today (only those three and the Toolkit are repacked on 2026-10-21): the code declares components, the
  engine packs.
- **T11 — A stale entry in development: warn and fall back on the sources** for that run (§ A stale entry): the log
  says which entry and why, the component takes the pixel path (`Images`, repack, cache), so the image shown is right.
  The engine NEVER rewrites `GPUTextures`: re-baking stays an explicit gesture of the tool. A release, without its
  sources, serves the entry as it is.
- **T12 — The vocabulary is ambientCG's** (most of our sources come from ambientCG; widely used by tools and banks):
  `Color`, `Opacity`, `NormalGL`, `NormalDX`, `Roughness`, `Metalness`, `AmbientOcclusion`, `Displacement`, `Emission`.
  `NormalGL` is the engine's Khronos convention; `NormalDX` (Direct3D) exists for SOURCES only — the tool converts it,
  `GPUTextures` refuses it.
- **T13 — In `Images` the convention is LAX** (owner): `Images` holds images in the basic sense — sprites, UI,
  heightmaps, cursors as well as material sources. An image named `<Name>.<Purpose>.<extension>` with a vocabulary
  token is RECOGNISED as a material source (a material can then be built from a texture-set name, the tool finds its
  sources without a recipe); any other image stays an ordinary image — never refused, never reported. The strict
  checks and refusals are `GPUTextures`' alone. A renaming pass of the material sources is projet-alpha's item
  `source-images-pbr-naming-pass`.
- **T14 — The engine only writes its cache** (owner): the data it ships with — `Images`, `GPUTextures`, every store —
  is READ-ONLY to it; in a released product nobody reworks the shipped data. The bake tool is a development tool: it
  writes where it is told (a development data directory included), never the engine at run time.
- **T15 — `GPUTextures` is the predominant store of a finished product** (owner): it keeps the work of sending textures
  to the GPU minimal at the customer's. What cannot be baked ahead (procedural images, dynamic content, an asset without
  a bake) stays in `Images` and is processed by the engine at the customer's, its cache at work.

## The Purpose vocabulary

The tokens are ambientCG's words (T12). For the canonical layout (graphics § 39):

| File | Format | Channels |
|---|---|---|
| `Grounds/Sand002.Color-Opacity.ktx2` | BC7 sRGB | RGB albedo, A opacity |
| `Grounds/Sand002.Color.ktx2` | BC7 sRGB | RGB albedo (opaque) |
| `Grounds/Sand002.NormalGL.ktx2` | BC5 | XY (Z reconstructed) — the normal stays alone (D6) |
| `Grounds/Sand002.AmbientOcclusion-Roughness-Metalness-Displacement.ktx2` | BC7 UNORM | R G B A |
| `Grounds/Sand002.Roughness.ktx2` | BC4 | one component |
| `Grounds/Sand002.Emission.ktx2` | BC7 sRGB | emission colour |

Guardrails (each one a refusal, logged with the file and the reason):
- every token belongs to the vocabulary, none repeated;
- the tokens fill the format's channels exactly: a colour token takes three channels, the normal two (BC5) and stands
  alone, every other token one channel;
- one colour space per file: colour tokens need an sRGB format and never share a file with linear data;
- the normal follows the engine's Khronos convention (+Y up): a Direct3D normal is converted by the tool, never shipped;
- the container is valid, its format one the device uploads (or transcodes), the mip chain complete, the size within
  the device's limits.

## Resolution, per material component

1. `<TextureName>` = the material's `TextureSet` if it declares one, else its name (T4).
2. The `GPUTextures` entries of that name are indexed by the components their Purpose lists. A component they cover is
   read from that file and that channel (the material's one binding per distinct texture, graphics § 39 D1, still
   applies).
3. A component no entry covers falls back on the material's own declaration: its image in `Images` through the pixel
   path (T5), or its value.
4. D5 (skipped levels per kind) and `MaxDimension` apply to a `GPUTextures` entry for free: the upload starts lower in
   the shipped chain.

## Production (T6)

The existing pieces — `EmeraudeTextureBake` (one canonical texture from its channel sources: `PackRecipe`,
`TextureCompressor`, the mip chain, `KTX2Writer`) and `tools/gltf-bake-textures.py` (a glTF's textures: a GLB, or a
`.gltf` and its files since L6) — grow into a
tool that reads a MATERIAL definition (its components and their images) and writes the material's `GPUTextures`
entries, named by the convention. Later, possibly: a console / MCP command that bakes a material loaded in the running
engine.

## Delivery modes

| Mode | `GPUTextures` | `Images` | Load-time texture work |
|---|---|---|---|
| Finished product | every material | not shipped (or sources only) | read + inflate (+ transcode for Basis) + upload |
| Development | empty or partial | sources | decode + repack + BC encode on a cache miss, decode + repack on a hit (D7 would skip it) |
| Mixed | baked materials | the rest | each component by its own path |

## A stale entry

An entry is a computation frozen when it was baked. It goes STALE when one of its ingredients changes afterwards,
while T5 keeps serving it first, silently:
1. **its sources** — an image in `Images` repainted;
2. **its recipe** — the material feeds another image to a component, or a normal now needs the Direct3D flip;
3. **the engine's pipeline** — the blocks a source compresses to changed (the Kaiser mip filter and the colour dilation
   of 2026-10-21 left `Sponza.baked.glb` behind until it was baked again) — the `TextureCache::Version` of a bake.

Detectable when the tool writes a fingerprint of the three into the container (KTX2 key/value data) and the engine,
in development, compares it with the sources it has. A release ships no sources: nothing to compare, the entry is
served.

## Implementation

The lots: **L1** the store, the name and its guardrails; L2 the resolution for JSON materials; L3 the bake tool; L4 the
fingerprint and the stale-entry fallback; L5 code-built materials (T10, T4); L6 `.gltf` assets baked in place (T9
revised) and the alpha coverage a container records.

**L1 (2026-10-21)**
- emeraude-base `PixelFactory/TextureName` (owner: the parser lives in the base, tested): `TextureName::parse(name,
  TextureStore::GPU | Sources)` splits `<TextureName>.<Purpose>` at the last dot of the last path component, reads the
  nine ambientCG tokens (case-sensitive; any other refused), lays them on consecutive channels from R (a colour three, a
  normal two, the rest one; four at most) and checks the rules (the normal alone, a colour first, a colour file holding
  only the opacity in A, no `NormalDX` in the GPU store). Every refusal is a `TextureNameError` with a log sentence
  (`to_cstring()`). 7 unit tests (`PixelFactoryTextureName.*`).
- `CompressedImageResource` reads the `GPUTextures` store (it read `Images`). A file loaded from the store is refused
  when its name is not valid, or when its purpose needs more channels than its format holds (`KTX2Decoder::channelCount()`:
  BC4 1, BC5 2, BC1 RGB / BC6H / ETC2 RGB 3, BC1 RGBA / BC2 / BC3 / BC7 / ETC2 RGBA / ASTC 4). A glTF's embedded image
  is loaded from memory and not checked.
- `ImageResource` refuses a GPU container (`.ktx2`, `.basis`, `.dds`) with a message pointing to `GPUTextures` (T8).
- Verified (Linux, a test store under `--add-data-directory`, loaded through the console's `loadResource()`):
  `Test/Good.Color-Opacity` (BC7) and `Test/Grey.Roughness` (BC4) Loaded; `Test/Mismatch.Color-Opacity` (a BC4 payload:
  "needs 4 channel(s), its format holds 1"), `Test/Bad.Rough` (unknown token), `Test/NoPurpose`, and `Images/Test/Wrong.ktx2`
  Failed with their reason; 0 `VUID-`. `sponza` (both options, embedded KTX2) still loads its 103 textures, 1886.8 MiB.
- Nothing USES a `GPUTextures` entry yet: L2.

**L2 (2026-10-21)** — the resolution for JSON materials. Owner decisions: an entry replaces the SOURCE of a component
the definition declares as a texture only (a value or an absent component is untouched: a bake never changes which
components a material has); a purpose two entries of a set serve makes the set ambiguous — refused whole, logged; the
key is `TextureSet`.
- `Graphics/GPUTextureSet`: `find(store, setName)` collects the entries `<setName>.<Purpose>` of the store (an entry
  naming the set but breaking the convention is reported and ignored), one slot per purpose; `texture()` creates the
  entry's `Texture2D` once, named `GPUTextures/<entry>`, with the identity its purpose gives (`enableSRGB()` for a colour,
  `enableNormalMap()` for `NormalGL`), loaded from the `CompressedImageResource`.
- `StandardResource::planGPUTextures()` runs before the repack: the set is `TextureSet`, else the material's name; each
  covered texture component reads its entry (`Name`, `SourceChannel` for a one-channel purpose; a `FlipNormalMapY` is
  dropped — the tool converts a Direct3D source); the repack then packs whatever still reads `Images`. The texture
  component finds a `GPUTextures/` texture as it finds a `Packed/` one.
- Verified (Linux, `relief` + `setGround("Grounds/Sand002")`, a test store with `Grounds/Sand002.Color`, `.NormalGL` and
  `.AmbientOcclusion-Roughness-Metalness-Displacement` baked by `EmeraudeTextureBake`): the material reads the three GPU
  textures (BC7 sRGB, BC5, BC7 UNORM through r / g / a), 16.006 MiB and 3 bindings as from the sources; none of its five
  source images is decoded, 3 BC encodes fewer; the BC5 levels are byte-identical to the runtime chain (12 / 12); the
  image equals the source path's within the noise (1.20–1.24 against 1.28, mean 130.17 both) once converged; the known
  `setGround` VUIDs only.
- ⚠️ **Compare the two paths CONVERGED**: with a cold cache the source path BC-encodes on `setGround`, so its textures
  arrive seconds after the GPU path's; a capture 8 s later saw a GI / TAA accumulation of another age — +0.66 brighter
  with RT, 0.38 in screen space (noise 0.14) — that vanished at 30 s.
- Not yet: D10's single fetch (graphics § 39) is kept for `Packed/` textures, whose readers share their UVs by
  construction; a GPU data texture is fetched once per reader (to extend when its readers' UV transforms are equal).
  The set is found by scanning the store's names once per material (`availableResourceNames()`, a copy): an index by
  set name if the store grows large.

**L3 (2026-10-21)** — the bake tool's material mode. Owner: a dev tool, it reads a data directory and writes where
`--output` says (T14).
- `EmeraudeTextureBake --data-directory <dir> (--material <name> | --materials <directory>) --output <dir> [--zstd N]`
  (`tools/texture-bake/MaterialBake.cpp`): reads `<dir>/data-stores/Materials/<name>.json` as the engine does (the maps
  declared as 2D textures, their image in `Images` by name, `SourceChannel`, the UV keys of D11, `FlipNormalMapY`,
  `EnableAlpha`, `TextureSet`), groups them with the engine's rule, and bakes one entry per group or lone map into
  `<output>/<TextureSet>.<Purpose>.ktx2` (the same steps as the single mode: `PackRecipe`, `TextureCompressor`,
  `KTX2Writer`). `--materials` walks a directory of the store recursively and prints a summary.
- **One rule**: `TexturePacker::plan()` (new, static) decides which maps share a canonical texture — the run-time packer
  and the tool call it. The tool COMPACTS the channels: a purpose lists only what the file serves (Sand002, its metalness
  a value: `AmbientOcclusion-Roughness-Displacement`, the height in B), where the run-time pack keeps fixed slots filled
  with 255. A lone grey map repeats its channel in R, G, B (BC4). An albedo read with its alpha (`EnableAlpha`) bakes
  `Color-Opacity` from its own A. Each entry name is checked with `TextureName` before writing.
- Verified (Linux): `Grounds/Sand002` → `Color`, `NormalGL`, `AmbientOcclusion-Roughness-Displacement` in 9.4 s,
  projet-alpha.data untouched; read by `relief`, the image equals the source path's within noise (1.28 = noise); a rerun
  bakes byte-identical files. `--materials Metals`: 13 definitions in 13.7 s, 12 baked (Grid003 `Color-Opacity` +
  `NormalGL` + `Displacement`, its other data values; Rusty001 `Color-Opacity`), Grid001 refused (its albedo image is
  missing from `Images`). The run-time packer after `plan()`: the 222 texture-cache blobs of a `relief` run
  byte-identical to before (keys and content).
- ⚠️ **`relief`, like `sponza`, does not always start at the same camera pose** (4 runs in a row 2026-10-21, the image 15–21
  apart): compare captures only after checking the framing, or compare what the GPU holds (texture-cache blobs,
  KTX2 levels, `getTextureMemory()`).
- The tool is `EXCLUDE_FROM_ALL`: the gate's builds do not compile it — build it with `--target EmeraudeTextureBake` in
  the GCC AND the clang trees before proposing a change to it.

**L4 (2026-10-21)** — the bake's fingerprint and the stale entry (T11). Owner decisions: a source is identified by
its CONTENT (FNV-1a 64 of its bytes); the check runs when the entry's sources are present, with a setting to turn it
off; an entry without fingerprint is served and reported; the pipeline version is `TextureCache::version()`.
- The tool writes the fingerprint into each container's key/value data, key `Emeraude.Bake`
  (`KTX2Writer::BakeFingerprintKey`): `{"version": 6, "purposes": {"Roughness": {"source": "Test/Grain-roughness",
  "channel": 0}, …}, "sources": {"Test/Grain-roughness": "<16 hex digits>", …}}` — a colour records no channel, a normal
  its `flip`.
- `GPUTextureSet::find()` reads it from the header and the key/value block only (`KTX2Decoder::readKeyValue()`, a bounded
  hand parser: no image is read); `GPUTextureSet::check()` decides per component: `Unverifiable` when a source file is
  absent (a product shipped without its sources — served), else `Stale` when the version differs, when the component's
  declared image / channel / flip differs from what its purpose was baked from, or when a source's content changed —
  the component then keeps its source (the repack takes it), the engine never rewrites `GPUTextures` (T14). A finding is
  reported once per entry and reason. `GPUTextureSet::sourceHash()` is the one hash of the tool and the engine.
- **`Core/Graphics/Texture/CheckGPUTextures`** (default true): false serves every entry as it is.
- Verified (Linux, `relief`, a test data directory with `Test/Grain` = Sand002's sources, baked by the tool): fresh → the
  three GPU textures, no report; `Test/GrainRecipe` (same set by `TextureSet`, another albedo image) → "its Color was
  baked from 'Test/Grain-color', the material now reads 'Grounds/Sand002-color'", the albedo alone from its source; one
  byte appended to a source → "its source 'Test/Grain-roughness' changed since the bake", AO / roughness / height from
  their sources (repacked), colour and normal still GPU, reported once; that source deleted → the entry served; the L3
  bake (no fingerprint) → served, one report per entry. Only the known `setGround` VUIDs.
- ⚠️ Hashing reads every source of an entry at each development load (Sand002: four files, ~20 MB) — not decoded, once
  per planning; a product without its sources skips it.

**L5 (2026-10-21)** — code-built materials (T4, T10). Owner decision: setters BY SOURCE, the texture setters kept.
- `Material::TextureSource` {image (an Images name), channel, UV set, UV scale, UV offset, Direct3D normal flip} and the
  overloads `setAlbedoComponent(source, enableAlpha)`, `setOpacityComponent`, `setNormalComponent`,
  `setAmbientOcclusionComponent`, `setRoughnessComponent`, `setMetalnessComponent`, `setHeightComponent`,
  `setAutoIlluminationComponent`, plus `setTextureSet()`: a declaration is RECORDED in the JSON definition format, and
  resolved by the JSON code when the material finishes loading — `planGPUTextures()`, `planPackedTextures()`, then the
  component's own `parse…Component()` and its dependency. A texture given explicitly (the former setters) is used as it
  is. The roughness has no inversion here, as in JSON. A component set twice is refused (logged).
- The hook: `Resources::ResourceTrait::onLoadFinishing()` (new, virtual, default true), called by
  `setLoadSuccess(true)` before the dependencies are checked — the last moment a resource adds a dependency.
- Verified (Linux, projet-alpha's `relief`, its own ground converted — projet-alpha's builtin docs § 6e): from the images
  the same textures as before the conversion (`Pavement006-color_c#sRGB` BC7 sRGB, `#normal` BC5, the height; 2.6709 /
  3.3398 MiB); with a bake of `Grounds/Pavement006` the three `GPUTextures/` entries, fresh; a temporary ambient
  occlusion declared from the height image → "2 maps packed into 'Packed/Data/…height.r|#255|#255|…height.r'" (the
  hidden repack, AO r, height a); a missing image → the store's default texture with a warning, as in JSON, a clean
  exit. 0 `VUID-` (no `setGround`).

**L6 (2026-10-22)** — a `.gltf` baked in place (T9 revised), and the alpha coverage of a baked container.
- `tools/gltf-bake-textures.py IN.gltf [MANIFEST.json --sources DIR | --flip-normal-y] --baker PATH --out OUT.gltf
  [--textures DIR]`: the containers are written as files in `--textures` (default: the output's name, `.textures` for
  its extension), named `<material>.<glTF slot>.ktx2`, the images point to them; the buffers are not touched, their uris
  rewritten relative to the output. Without a manifest the recipes are read from the glTF itself: each textured slot its
  own image file, channel for channel; `metallicRoughness` and `occlusion` share one image (R the occlusion's R, G B the
  metallicRoughness texture's, 255 for a slot the material lacks — the canonical layout of § 39); refused when the two
  read different `texCoord` sets. `--flip-normal-y` for an asset in the Direct3D convention (Intel's Sponza packages).
  The GLB mode is unchanged (a manifest), its output byte-identical to `Sponza.baked.glb`.
- Refused BEFORE anything is baked: a buffer or the textures directory outside the output's directory (the loader
  confines every uri to the asset's directory, owner decision 2026-10-18), an embedded or already-compressed image
  without a manifest, a missing image file. A container of the textures directory the output does not use is reported,
  never deleted.
- The tool's cache key now holds the CONTENT of the sources (SHA-256) beside the recipe, the baker's hash and the zstd
  level: a repainted source is baked again on the next run (before: only a recipe or baker change was seen).
  Owner decision: no fingerprint checked at load for a glTF (engine item `gltf-baked-textures-fingerprint`).
- **The alpha coverage a container records** (owner decision 2026-10-22). Found by L6: a block-compressed image cannot be
  inspected, so `Texture2D::isBinaryAlphaMask()` answered "not a mask" for every KTX2 — a blended material whose alpha
  is a binary mask (Sponza's cypress leaves, 99.86 % binary) stayed BLEND where the source path promotes it to a cutout
  (`StandardResource::promoteBinaryCoverageToCutout()`). The bake now measures it on the packed pixels with the runtime's
  rule (`AlphaCoverage::isBinaryMask()`, before the compression) and `KTX2Writer` records it under
  `Emeraude.AlphaCoverage` = `binary` (`AlphaCoverageKey`, `BinaryAlphaCoverage`; absent otherwise). `KTX2Decoder::
  decodeCompressed()` reads it (`Result::binaryAlphaMask`, through the new in-memory `readKeyValue(bytes, key)`),
  `CompressedImageResource::isBinaryAlphaMask()` and `Texture2D` answer it. Every KTX2 path gains it: `GPUTextures`, a
  baked GLB, a baked `.gltf`. A container baked before 2026-10-22 (or a Basis Universal one) has no key: not a mask, as
  before — `Sponza.baked.glb` included (its demo forces `LeafSpring` to a cutout by name, `cutoutMaterialNames`).
- libFuzzer target `fuzz_ktx2` (`src/Fuzzing/`): `readKeyValue()` over a container in memory, both keys; 779 million
  runs in 180 s from 13 container headers, 0 crash (closes L4's follow-up).
- Verified (Linux, `relief` + the console's `loadGLTF()`, a copy of Intel's cypress package): the four sidecars
  byte-identical to the cypress images of `Sponza.baked.glb` (same recipes); loaded "4 of 4 image(s) kept
  block-compressed", 182.58 MiB / 27 bindings as from the PNG sources, their 13 × 4 levels byte-identical to the
  runtime's texture-cache blobs; with the alpha key the image equals the source path's (0.16 against a run-to-run noise
  of 0.40, spectrum 0.99–1.02) where it was 0.83 apart without it (the leaves blended: spectrum 0.64 in the highest
  band); a JSON material `Test/Leaf` (albedo + binary opacity, so BLEND) promoted on both paths, its `GPUTextures` entry
  `Test/Leaf.Color-Opacity` against the sources 0.06 apart. 0 `VUID-` (the known `setGround` ones only).

**Validated on the three OS (2026-10-22**, base 145fda6, engine 3f8feeff, projet-alpha d648e28b): `relief`'s
code-built ground (Pavement006 BC7 sRGB + BC5), its bake read from a `GPUTextures` test store (image equal to the source
path's: macOS 0.30, Windows 1.31 against a run-to-run 1.39; the parallax option reads the BC4 displacement too), `sponza`
on both options unchanged, and L6 on `Models/IridescenceMetallicSpheres` (glTF-Sample-Assets: `ExternalData` is
git-ignored, the peers have no cypress) — one container with `Emeraude.AlphaCoverage`, forward slashes in its uris,
"1 of 1 image(s) kept block-compressed", its BLEND material promoted to a cutout as from the PNG; `LightsPunctualLamp`
refused before baking (a texture inside `KHR_materials_transmission`). 0 `VUID-` (the known `setGround` ones aside).
- **macOS** (M6, MoltenVK 1.4.2): Apple clang paranoid 0 warning, `-Wdocumentation -Werror` clean on the 8 touched TUs,
  base 2721 passed / 6 skipped / 0 failed; L6 0.142 apart (p99.9 2); `sponza` 1887.4 MiB / 162 bindings both options;
  no MoltenVK complaint about BC4 / BC5 / BC7 KTX2.
- **Windows** (RTX 3500 Ada, MSVC /W4 /WX): Release and Debug 0 warning, the tool too, base 2722 passed / 3 skipped /
  3 disabled / 0 failed in both; L6 0.73–0.78 apart against 0.70 within a run; `sponza` with the RT lane 1886.8 MiB /
  162 bindings both options. The tool prints its output paths with mixed `\` and `/` (cosmetic).
- **Linux** (RTX 3070 Ti): the numbers of each lot above.

## Open work

Engine items (`docs/todo/`): `texture-purpose-vocabulary-beyond-ambientcg` (the open question below),
`gpu-textures-container-agnostic-decoder` (T7), `images-material-source-recognition` (T13),
`gpu-data-texture-single-fetch` (D10 for a GPU data texture), `gpu-textures-set-index`, `material-bake-parallel`,
`gltf-baked-textures-fingerprint`. projet-alpha: `source-images-pbr-naming-pass`.

## Open questions (owner)

1. **Tokens beyond ambientCG's set**: the components it does not name (clear coat, sheen, transmission, specular
   colour, iridescence, anisotropy, subsurface…) need tokens in the same style; to be listed with the owner when the
   parser is written.
