## 4. Caching Architecture

### Pipeline Selection Flow

When a `RenderableInstance` needs to be drawn, the system follows this exact sequence:

```
┌─────────────────────────────────────────────────────────────────────┐
│ 1. RENDER REQUEST                                                   │
│    RenderableInstance wants to draw on a RenderTarget               │
│    File: Graphics/RenderableInstance/Abstract.cpp                   │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 2. RENDERABLE-LEVEL CACHE                                           │
│    File: Graphics/RenderableInstance/Abstract.cpp                   │
│                                                                     │
│    → Builds ProgramCacheKey with:                                   │
│      - programType, renderPassType, renderPassHandle (!)            │
│      - layerIndex, isInstancing, isLightingEnabled...               │
│                                                                     │
│    → Looks up Renderable::m_programCache[cacheKey]                  │
│                                                                     │
│    ✓ HIT  → Use this program, skip to step 5                        │
│    ✗ MISS → Continue to step 3                                      │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 3. RENDERER-LEVEL PROGRAM CACHE                                     │
│    File: Graphics/Renderer.cpp                                      │
│                                                                     │
│    → Generator (SceneRendering, etc.) computes key via              │
│      computeProgramCacheKey() which includes:                       │
│      - renderPassHandle (!), isCubemap, renderableName              │
│      - layerIndex, renderPassType, flags...                         │
│                                                                     │
│    → Looks up Renderer::m_programs[generatorCacheKey]               │
│                                                                     │
│    ✓ HIT  → Use this program, skip to step 5                        │
│    ✗ MISS → Continue to step 4                                      │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 4. SHADER PROGRAM GENERATION                                        │
│    Files: Saphir/Generator/*.cpp                                    │
│                                                                     │
│    → Generator::onGenerateShadersCode() creates shaders             │
│      (vertex, fragment, geometry...)                                │
│    → Generator::onCreateDataLayouts() creates descriptor layouts    │
│    → Compiles SPIR-V shaders                                        │
│    → Stores in Renderer::m_programs                                 │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 5. RENDERER-LEVEL PIPELINE CACHE                                    │
│    File: Graphics/Renderer.cpp → finalizeGraphicsPipeline()         │
│                                                                     │
│    → GraphicsPipeline::getHash(renderPass) computes hash with:      │
│      - renderPassHandle (!)                                         │
│      - shader stages, vertex input, topology                        │
│      - rasterization, depth/stencil, color blend states...          │
│                                                                     │
│    → Looks up Renderer::m_graphicsPipelines[pipelineHash]           │
│                                                                     │
│    ✓ HIT  → Use this pipeline                                       │
│    ✗ MISS → Continue to step 6                                      │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 6. VULKAN PIPELINE CREATION                                         │
│    File: Vulkan/GraphicsPipeline.cpp                                │
│                                                                     │
│    → vkCreateGraphicsPipelines() with the specific RenderPass       │
│    → Stores in Renderer::m_graphicsPipelines                        │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 7. DRAW CALL                                                        │
│    → vkCmdBindPipeline(pipeline)                                    │
│    → vkCmdDraw(...)                                                 │
└─────────────────────────────────────────────────────────────────────┘
```

> [!CRITICAL]
> **renderPassHandle is MANDATORY in ALL cache keys!**
>
> Vulkan pipelines are tied to specific render passes. A pipeline created for render pass A
> (e.g., offscreen 1 sample) CANNOT be used with render pass B (e.g., main view 4 samples).
>
> ALL THREE cache levels must include renderPassHandle:
> 1. `ProgramCacheKey::renderPassHandle` (Renderable level)
> 2. `Generator::computeProgramCacheKey()` (Renderer program cache)
> 3. `GraphicsPipeline::getHash(renderPass)` (Renderer pipeline cache)
>
> Missing renderPassHandle in ANY cache causes Vulkan validation errors:
> - "sample count mismatch"
> - "format mismatch"
> - "VkRenderPass incompatible"

### Renderer-Level Caches

The `Renderer` maintains global caches for performance optimization:

| Cache | Member | Key Source | Purpose |
|-------|--------|------------|---------|
| Programs | `m_programs` | `Generator::computeProgramCacheKey()` | Saphir Program cache (biggest gain) |
| Pipelines | `m_graphicsPipelines` | `GraphicsPipeline::getHash(renderPass)` | Vulkan GraphicsPipeline cache |
| Samplers | `m_samplers` | `hashSamplerCreateInfo()` — the **content** of the `VkSamplerCreateInfo` | Texture sampler cache |

> [!CAUTION]
> **The sampler cache key is the create-info CONTENT, not the identifier.** It used to be the
> identifier string, which meant the FIRST caller of a given name imposed its sampler on every
> later one — silently, because `getSampler()` skipped the setup lambda on a cache hit. Two call
> sites shared the name `"ShadowMap"` while requesting **opposite** addressing, so every real shadow
> map sampled with `DummyShadowTexture`'s `CLAMP_TO_EDGE` instead of the `CLAMP_TO_BORDER` +
> opaque-white border it asked for. Visible symptom: a broad black band past the edge of a
> directional shadow map's coverage (the border texel ring smeared over the whole exterior), and a
> point-light `samplerCube` fed a compare-enabled sampler — undefined per spec, working only by
> driver leniency. Consequence of the fix: **the setup lambda now runs on every call**, so it must
> stay cheap and side-effect-free, and the identifier is a debug label only — differentiating two
> samplers no longer requires encoding anything in the name.

**Statistics** available at shutdown via `programBuiltCount()`, `programsReusedCount()`, `pipelineBuiltCount()`, `pipelineReusedCount()`.

> [!CRITICAL]
> **These caches OWN their objects; consumers only borrow.** A `shared_ptr` handed out by
> `getSampler()` (and likewise programs/pipelines/layouts) is shared by many users. A consumer's
> teardown must **release its reference** (`m_x.reset()`) — **never** call `destroyFromHardware()`
> on it. The Renderer destroys each cached object **once**, at shutdown (`onTerminate`). Destroying
> a cached sampler from a texture/overlay teardown invalidated it for every other user
> (`VUID-vkDestroySampler-sampler-01082` + invalid descriptors). Fixed Jun 2026 across all
> `TextureResource` types and `Overlay::Surface`; see [`docs/caution-points.md`](../../caution-points.md)
> and [`docs/multi-scene-resource-ownership.md`](../../multi-scene-resource-ownership.md).

> [!NOTE]
> **`Texture2D`'s `-<U><V>` name suffix is now REDUNDANT, and kept as a debug label.** This block
> used to say the opposite of the one above — "the identifier IS the sampler cache key, anything
> that must distinguish two samplers has to appear in the NAME" — and that was true when it was
> written (Aug 2026) and false a day later, once the key became the create-info content. Two
> contradicting CAUTION blocks stood in this file until the second one was reread.
>
> The history is still worth keeping, because it is what the suffix is for: `Texture2D` used the
> bare identifier `"Texture2D"` for every 2D texture in the engine, so when glTF sampler addressing
> was wired in, whichever texture happened to be created first would have imposed its wrap modes on
> all the others. The suffix (`R`/`M`/`C`, appended only when the modes are not the default
> repeat/repeat) fixed that under the old keying. Under content keying the two samplers separate on
> their own, so the suffix no longer carries correctness — it only makes the two entries legible in
> a capture. ⚠️ Do not restore the old rule: putting distinguishing state in the NAME is now
> pointless, and a name that varies per material fragments nothing but the debug labels.

### Every sampled texture has a mip chain — the animated ones too (Sep 2026)

`Texture2D` builds `min(Image::getMIPLevels(w, h), Core/Graphics/Texture/MipMappingLevels)` levels, and since
2026-09-25 so does `AnimatedTexture2D` (one chain per frame layer; the upload blits every layer's,
`ImageTransferOperation`). It had ONE level: the ocean's animated normal map was sampled at full resolution at
every distance and the sea read as noise past a few dozen metres (owner: "the filtering on the water is
disgusting"). A good sampler (linear, anisotropy 8) cannot hide a missing chain. ⚠️ `CubemapMovieResource`
is unchecked.

### Texture addressing comes from the ASSET, not from a global default

`TextureResource::Abstract` carries a `WrapMode` per axis (`setWrapModes()`, defaults repeat/repeat
— the Vulkan **and** glTF default), consumed once when the sampler is built and never revisited, so
it must be set BEFORE creation on hardware, exactly like `enableSRGB()`.

⚠️ Ignoring an asset's addressing does not fail loudly — the texture simply TILES where the asset
asked for a clamp, which silently corrupts anything authored with a border. Measured on the Khronos
`TextureTransformTest`, whose scaled quads repeated instead of showing their grey border once; the
fix moved **51 %** of those two quads' pixels while leaving the quads whose UVs stay inside [0,1]
**bit-identical**. That last point is the useful one for judging regression risk: addressing is
observable ONLY where UVs actually leave [0, 1], so content that stays in range renders identically
whatever it declares (`MetalRoughSpheres` declares clamp on both axes and does not move).

### Persistent (on-disk) Caches — the Renderer owns the pipeline-cache and texture-cache I/O

The three caches above live for **one run**. Three caches survive across runs, and the disk I/O of
**two** of them is implemented in this directory:

| On-disk cache | Setting | Default | Owns the object | Does the disk I/O |
|---|---|---|---|---|
| `VkPipelineCache` (driver blob) | `Core/Graphics/Shader/EnablePipelineCache` | **true** | `Vulkan::Device` | **`Graphics::Renderer`** |
| SPIR-V binary cache | `Core/Graphics/Shader/EnableBinaryCache` | **true** (flipped Aug 2026) | `Saphir::ShaderManager` | `Saphir::ShaderManager` |
| BC7 texture cache | `Core/Graphics/Texture/EnableTextureCache` | **true** (added Aug 2026; it had NO setting before, it was on whenever the GPU reported `textureCompressionBC`) | **`Graphics::TextureCache`** (Renderer sub-service) | **`Graphics::TextureCache`** |
| Generated-GLSL dump (NOT a cache) | `Core/Graphics/Shader/EnableSourceCodeDump` | `false` | `Saphir::ShaderManager` | a DUMP — nothing ever reads it back; renamed Aug 2026, the old `EnableSourceCodeCache` key is silently ignored (no migration) |

**The part that lives here.** `Renderer::loadPipelineCache()` runs right after the device is
acquired — the cache **must exist BEFORE any pipeline is created**, so do not move that call —
and `Renderer::savePipelineCache()` runs in `onTerminate()` after `waitIdle()`, when every
pipeline the run compiled is in it. Measured on `material-debug` (294 graphics pipelines,
RTX 3070 Ti): driver disk cache OFF = **5 702 ms**, driver disk cache OFF but the engine cache
restored from disk = **31 ms** (182×, 7.4 MB blob); with the driver cache active, 33 ms. The blob
is never handed to the driver raw — the application header, the load marker and the
write-then-rename are each answering a real crash mode: full rationale in
[`src/Vulkan/AGENTS.md`](../../../src/Vulkan/AGENTS.md) § "VkPipelineCache".

**The binary cache, for context only** — no Graphics code touches it. Same demo, 232 shader
modules: **393 ms** with the cache OFF → **10.3 ms** warm (38×, 383 ms saved), and the cold run
that WRITES the 232 blobs costs 391 ms, i.e. nothing — there is no first-launch penalty, which is
why it is now on by default. It is safe to leave on because an application header, including a
**toolchain identity hash** (glslang version + SPIR-V generator version + client/target
environment pair + engine version), is validated in full before any byte reaches Vulkan. See
[`src/Saphir/AGENTS.md`](../../../src/Saphir/AGENTS.md).

**The BC7 texture cache, also implemented here** — `Graphics::TextureCache`, a sub-service of the
`Renderer`, stores the BC7 mip chains produced by the pixel path in
`~/.cache/<app>/texture-cache/` with a `.bc7cache` extension (⚠️ **not** `TextureCache/` — several
documents have claimed that directory name and it has always been wrong). Same demo
(`material-debug`, all 10 options, RTX 3070 Ti, Release): cold cache = **231 mip-level
compressions, 7 705 ms** of BC7 compression → warm cache = **0 compressions, 0 ms**. That makes it
the single biggest on-disk saving of the three — more than the `VkPipelineCache` (5 702 → 31 ms)
and about twenty times the SPIR-V binary cache (393 → 10.3 ms). Full contract: § "The two BC7
sub-services" below.

`--clear-renderer-cache` (renamed from `--clear-shader-cache`) clears **all three** on-disk
renderer caches: the SPIR-V binary cache, the `VkPipelineCache` blob, and — since Aug 2026, in
`TextureCache::onInitialize()` — the BC7 texture cache.

⚠️ **An absent cache file is the nominal first-launch state, not an error.** `loadPipelineCache()`
checks `std::filesystem::exists()` before reading, and the `--clear-renderer-cache` branch guards
each `IO::eraseFile()`; without that, the one launch where the blob and the `.loading` marker are
*supposed* to be missing printed an IO error per file — on every fresh install, since the cache is
enabled by default. See [`docs/caution-points.md`](../../caution-points.md) § "Flipping a
default to ON runs a path nobody had ever run".

### Renderable-Level Cache

Each `Renderable::Abstract` maintains a program cache per render target:

| Member | Type | Purpose |
|--------|------|---------|
| `m_programCache` | `Map<RenderTarget → Map<ProgramCacheKey → Program>>` | Programs for this Renderable |
| `m_programCacheMutex` | `std::mutex` | Thread-safe access |

**Key principle**: Programs are cached at Renderable level, not per-instance. All `RenderableInstance` objects sharing a `Renderable` share its cached programs.

**ProgramCacheKey** identifies a unique program configuration:
- `programType`: Rendering, ShadowCasting, TBNSpace
- `renderPassType`: Ambient, directional, point, spot lights
- **`renderPassHandle`**: VkRenderPass handle (CRITICAL for pipeline compatibility)
- `layerIndex`: Material layer
- `isInstancing`: Unique vs Multiple rendering
- `isLightingEnabled`, `isDepthTestDisabled`, `isDepthWriteDisabled`: Instance flags

See: `Renderable::Abstract::findCachedProgram()`, `cacheProgram()`, `ProgramCacheKey.hpp`

> [!CRITICAL]
> **A shared cached program does NOT make an instance ready.** Some descriptor sets the
> sealed pipeline layout demands are **per instance** — today the skeletal skinning SSBO
> (`SetType::PerModel`). `isReadyToRender()` / `isReadyToCastShadows()` therefore also test
> `isMissingSkinningResources()`, and `getReadyForRender()` /
> `getReadyForShadowCasting()` both call `prepareSkinningResources()` before generating
> anything. Skipping that test let a second instance of a skeletal mesh be drawn without its
> `PerModel` set — which shifted every following set one slot down (Aug 2026, see
> [`docs/caution-points.md`](../../caution-points.md) § Vulkan Validation and
> [`src/Saphir/AGENTS.md`](../../../src/Saphir/AGENTS.md) § "Descriptor set binding contract").

### Window Resize and Render Pass Handle Invalidation

> [!CRITICAL]
> **When the window is resized, the swapchain is recreated with a NEW render pass handle!**
>
> This means ALL cached programs for the main view become stale because their `ProgramCacheKey::renderPassHandle` no longer matches the current render pass.

**Problem scenario (before fix):**
1. Window resize → swapchain recreated → new render pass handle
2. `isReadyToRender()` checked `hasAnyCachedPrograms()` → returned `true` (old programs exist)
3. `render()` tried to find program with NEW handle → failed
4. Error: "There is no suitable render program for the renderable instance"

**Solution:**
The `isReadyToRender()` function now validates that cached programs have a matching render pass handle:

```cpp
bool Abstract::isReadyToRender (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) const noexcept
{
    // ...
    const auto renderPassHandle = reinterpret_cast< uint64_t >(renderTarget->framebuffer()->renderPass()->handle());
    return m_renderable->hasAnyCachedProgramsForRenderPass(renderTarget, renderPassHandle);
}
```

**Code references:**
- `Renderable/Abstract.cpp:hasAnyCachedProgramsForRenderPass()` - Validates render pass handle against cached keys
- `RenderableInstance/Abstract.cpp:isReadyToRender()` - Uses handle validation
- `Vulkan/SwapChain.cpp:recreate()` - Creates new render pass on resize
