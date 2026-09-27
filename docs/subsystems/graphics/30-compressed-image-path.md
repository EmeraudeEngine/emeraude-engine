## Compressed image path (KTX2 / `KHR_texture_basisu`, Aug 2026)

### Two ways to reach BC7 — the source decides, not a setting

| | Source | CPU work at load | Disk cache | Uncompressed copy in RAM |
|---|---|---|---|---|
| **(a) Pixel path** | `ImageResource` (PNG, JPEG, procedural) | full BC7 encode (bc7enc, via `renderer.textureCache().getOrCompress()`) | yes (`Graphics::TextureCache`, `.bc7cache` on disk) | **yes** — RGBA8 level 0 |
| **(b) Compressed path** | `CompressedImageResource` (KTX2) | a block→block transcode | no, and none needed | **never** |

The colour space is picked by the **texture's** sRGB flag, never by the source container: path (a)
creates the image as `VK_FORMAT_BC7_SRGB_BLOCK` or `VK_FORMAT_BC7_UNORM_BLOCK`, path (b) runs the
decoded linear format through `KTX2Decoder::sRGBFormat()` — also BC7 for anything the transcoder
touched, see § "Classes" for the pass-through case.

`Texture2D::createTexture()` is now a dispatcher: `createFromCompressedData()` or
`createFromPixelData()`, then the shared image-view + sampler tail. Exactly one of `m_localData`
/ `m_compressedData` is non-null.

**The memory argument, in numbers.** A 4096×4096 texture costs ~22 MiB as BC7 *with its full mip
chain*, against ~89 MiB for the RGBA8 level 0 **alone** that path (a) must materialise before it
can compress anything. On the compressed Sponza (84 images, all 4096², all UASTC+zstd) that is the
difference between a KTX2 payload of 1092 MiB read straight through, and 84 successive 89 MiB
decodes each followed by a bc7enc pass.

### The two BC7 sub-services (Aug 2026) — `TextureCompressor` + `TextureCache`

Both classes used to be **a grouping of statics** — the shape the owner does not want in the engine,
after the earlier pass removing needless singleton logic. They are now real sub-services of
`Graphics::Renderer`:

| Class | ClassId | Member | Reached by |
|---|---|---|---|
| `Graphics::TextureCompressor` | `"TextureCompressorService"` | `Renderer::m_textureCompressor` | `renderer.textureCompressor()` — **const &** |
| `Graphics::TextureCache` | `"TextureCacheService"` | `Renderer::m_textureCache` | `renderer.textureCache()` — **const &** |

Both derive from `EmEn::ServiceInterface`, are **value members** of the `Renderer`, are enrolled in
`Renderer::initializeSubServices()` into `m_subServicesEnabled`, and are terminated in reverse order
with every other sub-service. Neither failing is fatal: the compressor failing means textures are
not BC7-compressed, the cache failing means they are compressed at every launch.

> [!CRITICAL]
> **Declaration order is a constraint, not a style choice.** `m_textureCache` is declared **AFTER**
> `m_textureCompressor` in `Renderer.hpp` and initialised **after** it in
> `initializeSubServices()`, because the cache holds `const TextureCompressor & m_compressor` and
> uses it on every miss. Swapping the two declarations — or moving `m_textureCache` above the
> compressor — binds a reference to a not-yet-constructed member.

**`getOrCompress()` is the entry point — callers no longer orchestrate anything.**

```cpp
/* Graphics/TextureResource/Texture2D.cpp — createFromPixelData() */
const auto compressedMips = renderer.textureCache().getOrCompress(this->name(), m_localData->data(), mipLevels);
```

`TextureCache::getOrCompress(resourceName, pixmap, maxMipLevels)` does the disk lookup, compresses
through the `TextureCompressor` sub-service on a miss, and stores the result. The old
try / compress / store dance at the call site is gone. Two call sites were migrated:
`Texture2D::createFromPixelData()` (the cached mip chain, above) and
`CompressedImageResource::load()`, which builds the **default** 64×64 payload — a single level, no
mip chain — and therefore reaches the compressor directly
(`serviceProvider().graphicsRenderer().textureCompressor().compressSingle(pixmap)`), with no cache,
by design. That call is the ONE bc7enc pass on the compressed side and it encodes a procedural
fallback, not an asset: row (b) of the table above still holds for every real KTX2.

**All mutable static state is gone.** `TextureCompressor::s_initialized`, plus
`TextureCache::s_cacheDirectory` and `s_initialized`, no longer exist. The one-time
`bc7enc_compress_block_init()` moved into `TextureCompressor::onInitialize()`, so a caller can no
longer reach a compression method before the encoder is ready — the old static `initialize()` the
caller had to remember to invoke is **DELETED**, and forgetting it used to produce only a runtime
error log. The cache's directory is the `m_cacheDirectory` member and its readiness is the base
class `usable()` state. The pure private helpers `generateMip()` and `compressLevel()` moved to an
anonymous namespace in `TextureCompressor.cpp`. What remains static in the headers is
`static constexpr` constants plus two functions that are pure on their arguments and hold nothing:
`TextureCompressor::compressedSize()` (block arithmetic) and the private `TextureCache::cacheKey()`
(the content hash).

> [!WARNING]
> **The thread-pool parameter was dead and has been removed.** `compressLevel()` received a
> `Base::ThreadPool` and never used it; `compress()` and `compressSingle()` no longer take one.
> BC7 compression is **sequential per texture** — the parallelism comes from the resource manager
> loading several textures concurrently on different workers. Any document claiming compression is
> "parallelized across blocks using the engine ThreadPool" is **FALSE**; correct it where you find it.

#### The cache key was broken and is fixed (file format Version 1 → 2)

- **BEFORE:** `SHA256(resourceName | sourceFileSize | sourceModTime)`. But the caller passed, as
  `sourceFileSize`, the **decoded pixel byte count**, and as `sourceModTime`,
  `width * 1000000 + height`. The key therefore reduced to **name + dimensions**: repainting a
  texture without changing its size served the stale BC7 blob forever. The class documentation
  claimed file size and modification time — it described a mechanism that was not there.
- **AFTER:** **FNV-1a** (`Base::Hash::FNV1a`) over the **decoded pixels**, folded with `width`,
  `height` and `colorCount`. Content-addressed, correct by construction, and it needs no plumbing
  through `ResourceTrait`.

> [!CAUTION]
> **Changing the key scheme ORPHANS entries, it does not invalidate them.** Their filenames stop
> being produced, so they stay on disk unreachable rather than being detected as stale — no header
> check can catch what is never opened. `--clear-renderer-cache` is the remedy (**40** stale entries
> erased here when the key changed). `Version` stays **1**: the file FORMAT did not change, only the
> key, and a version number that moves for other reasons stops meaning anything.

#### Measured (`material-debug`, all 10 options, RTX 3070 Ti, Release)

| Run | BC7 compressions | Time spent compressing |
|---|---|---|
| Cold cache | **231** mip levels | **7 705 ms** |
| Warm cache | **0** | **0 ms** |

So the texture cache is worth **~7.7 s of load time** — more than the `VkPipelineCache`
(5 702 ms → 31 ms) and about twenty times the SPIR-V binary cache (393 ms → 10.3 ms). **Zero**
compressions on the warm run is also the proof that the content-addressed key is deterministic:
every texture found its entry. Build `-Werror` clean, 1967/1967 emeraude-base unit tests pass.

**On disk:** `~/.cache/<app>/texture-cache/`, extension `.bc7cache`
(`TextureCache::CacheDirectoryName` / `CacheFileExtension`).

> [!WARNING]
> Several documents claim `~/.cache/AppName/TextureCache/`. That path is **wrong and has always
> been wrong** — the code has always used a `texture-cache` sub-directory. Fix it where you see it.

### Classes

- **`Graphics::KTX2Decoder`** — stateless. `isKTX2()` (magic-number probe), `decodeCompressed()`
  → `{mips, VkFormat}`, `decodeToPixmap()` (the no-BC-support fallback), `sRGBFormat()`.
  Transcode target is **BC7**, because that is the one block format the engine supports. A KTX2
  that already carries a real `vkFormat` (nothing to transcode) is passed through untouched.
- **`Graphics::CompressedImageResource`** — an **opaque GPU payload**, deliberately. No
  `averageColor()`, no `isGrayScale()`, no per-pixel access, no `flipNormalMapY()`: answering any
  of them means decoding the blocks, which is exactly what the type exists to avoid. Code that
  needs to *inspect* pixels wants `ImageResource`.

> [!CAUTION]
> **The stored format is always the LINEAR variant, and that is load-bearing.** libktx derives the
> transcoded `vkFormat` from the container's transfer function, so an sRGB-tagged asset comes back
> as `*_SRGB_BLOCK`. The decoder normalises it back to linear, because the colour space is the
> **texture's** call — it knows the usage (albedo and emissive are sRGB, normal and ORM maps are
> not), the container does not. The blocks are bit-identical either way. Taking the container's
> word for it double-applies the sRGB curve on every ORM and normal map of the asset.

> [!WARNING]
> `isGrayScale()` and `averageColor()` **must** null-check `m_localData`, not just `isLoaded()` —
> a texture on the compressed path is fully loaded *and* has no pixmap.

### `Core/Graphics/Texture/MaxDimension` (default 4096)

Largest accepted mip dimension; 0 disables clamping. **Only honored by sources that ship a
ready-made mip chain** (i.e. KTX2), where clamping is free: the top levels are simply not kept,
nothing is resampled. Every halving divides the VRAM footprint by four — Sponza's 84 textures cost
~1.88 GiB at 4096, ~470 MiB at 2048, ~118 MiB at 1024. The default changes no behaviour; the knob
exists for the 8 GiB machine.

### Block formats and `Image::pixelBytes()` / `colorCount()`

Block formats are **absent** from both tables, which return 0 for them. That is not a latent bug
on this path: `Image::createFromCompressed()` never consults them — it is handed explicit per-level
byte counts. Do not "fix" the tables by giving a block format a per-pixel size; the honest answer
for BC7 is 1 byte per pixel *amortised over a 4×4 block*, which is not what those accessors mean.

> **`AbstractBackground::setLuminance()` is a RUNTIME knob since 2026-09-13.** The luminance has two
> consumers — the IBL scale pushed to the view buffers by `Scene::refreshAmbientLightProperties()`
> (`environmentLuminance`, the only factor applied to what comes out of the environment cubemap) and
> the emission of what is DRAWN. The second used to be fixed at load (the skybox material's emissive
> strength, part of the material identity); the protected hook `onLuminanceChanged(nits)` now lets a
> concrete background follow, and `SkyBoxResource` overrides it with
> `StandardResource::setEmissiveStrengthValue()` (dynamic property) on the material IT authored from
> the manifest — a caller-owned material (`load(material)`) stays as authored. Caller of record:
> `Scenes::Component::SkyFollowsSun` (a sky fading through twilight behind `SunCourse`). ⚠️ Changing
> the luminance does NOT re-bake the IBL: the bake is normalised, the scale is applied at sampling.

> **Material DYNAMIC properties reach the GPU since 2026-09-13.** `StandardResource`'s 39 runtime
> setters (`setRoughness`, `setOpacity`, `setAlbedoColor`, `setIBLIntensity`,
> `setEmissiveStrengthValue`, …) used to raise `m_videoMemoryUpdated` that nothing read — the
> material UBO was written once, by `create()`. Now they call `markVideoMemoryDirty()` (one
> registration per flush) → `Renderer::requestMaterialVideoMemoryUpdate()` (any thread) →
> `Renderer::flushMaterialVideoMemoryUpdates()` on the render thread, once per frame after the fence,
> through the virtual `Material::Interface::updateVideoMemory()`. ⚠️ The material UBO has ONE frame
> region: a value may be read one frame early by a frame still in flight — acceptable for a property,
> to be frame-partitioned the day a property must be exact per frame. `docs/caution-points.md`
> § "every dynamic material property was DEAD".
