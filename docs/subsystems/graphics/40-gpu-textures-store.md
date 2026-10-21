# The `GPUTextures` store — GPU-ready material textures (design, Oct 2026)

> Status: L1 implemented (2026-10-21, § Implementation), the rest DESIGN. Item `gpu-textures-store` (`docs/todo/`). Owner decisions 2026-10-21 (T1–T13
> below). Builds on graphics § 39 (packing, block formats, the mip
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
`TextureCompressor`, the mip chain, `KTX2Writer`) and `tools/gltf-bake-textures.py` (a glTF's textures) — grow into a
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
fingerprint and the stale-entry fallback; L5 code-built materials (T10, T4); L6 `.gltf` materials (T9).

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

## Open questions (owner)

1. **Tokens beyond ambientCG's set**: the components it does not name (clear coat, sheen, transmission, specular
   colour, iridescence, anisotropy, subsurface…) need tokens in the same style; to be listed with the owner when the
   parser is written.
