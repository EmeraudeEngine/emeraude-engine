## VkPipelineCache — the driver cache the engine now owns (Aug 2026)

`Vulkan::Device` owns one `VkPipelineCache`, handed to both `vkCreateGraphicsPipelines`
(`GraphicsPipeline.cpp`) and `vkCreateComputePipelines` (`ComputePipeline.cpp`) — every pipeline
in the engine goes through one of those two. `Graphics::Renderer` does the disk I/O:
`loadPipelineCache()` right after the device is acquired (it must exist BEFORE any pipeline is
created) and `savePipelineCache()` in `onTerminate()` after `waitIdle()`. Setting:
`Core/Graphics/Shader/EnablePipelineCache`, `DefaultPipelineCacheEnabled` = **`true`**
(`SettingKeys.hpp`) — this cache is **on by default**, and the Aug 2026 pass that flipped the
SPIR-V binary cache on left it untouched. On disk: `<cache>/pipeline-cache/pipeline.cache`
(+ the `pipeline.cache.loading` marker described below), via
`FileSystem::cacheDirectory("pipeline-cache")`.

`Device::pipelineCache()` returns `VK_NULL_HANDLE` when the setting is off or creation failed,
and `vkCreate*Pipelines` accepts that as "no cache" — which is why `GraphicsPipeline.cpp` and
`ComputePipeline.cpp` pass it unconditionally, with no null check and no second code path.

### Do not confuse it with the other shader caches

Four distinct mechanisms are commonly called "the shader cache". Only the `VkPipelineCache` (third
row) is owned by this layer; the three on-disk ones share the `Core/Graphics/Shader/…` prefix and
are all wiped by `--clear-renderer-cache`.

| What | Setting key | Default | Owner — read there, not here |
|---|---|---|---|
| Generated shader SOURCE — a **dump**, nothing ever reads it back | `EnableSourceCodeDump` | `false` | `Saphir::ShaderManager` — [`src/Saphir/AGENTS.md`](../../../src/Saphir/AGENTS.md) |
| Compiled **SPIR-V binaries** — skips glslang entirely on a hit | `EnableBinaryCache` | **`true`** | `Saphir::ShaderManager` — [`src/Saphir/AGENTS.md`](../../../src/Saphir/AGENTS.md) |
| **`VkPipelineCache`** — skips the DRIVER's pipeline compilation | `EnablePipelineCache` | **`true`** | `Vulkan::Device` + `Graphics::Renderer` — **this section** |
| In-memory reuse of ShaderModule / Program / GraphicsPipeline objects **within one run** | *(none — always on)* | — | [`docs/pipeline-caching-system.md`](../../pipeline-caching-system.md) |

⚠️ The first row's key was `EnableSourceCodeCache` until Aug 2026 (the dump was misnamed a cache).
There is **no migration**: an old key left in `settings.json` is dead JSON, silently ignored, so
anyone who had the dump enabled finds it off until they set the new key. Details in the owner doc.

The two disk caches cut **different** costs and neither substitutes for the other: the SPIR-V
cache removes GLSL→SPIR-V compilation, the pipeline cache removes SPIR-V→machine-code
compilation. Their validation headers are also separate — do not assume a fix to one covers the
other.

### Measured, on `material-debug` (294 graphics pipelines, RTX 3070 Ti)

| driver disk cache | engine pipeline cache | time in vkCreateGraphicsPipelines |
|---|---|---|
| on | — | 33 ms |
| **off** | — | **5 702 ms** |
| **off** | **restored from disk** | **31 ms** |

A 182× difference, and the third row is the point: the engine's own cache does the whole job
without the driver's. Blob size: 7.4 MB. ⚠️ Before this existed the engine passed
`VK_NULL_HANDLE` everywhere and was entirely at the mercy of the driver's cache — which is
per-machine, size-capped with eviction, invalidated by every driver update, and absent on some
drivers. That is a 5.7 s synchronous stall on a cold machine, in an engine that already has a
documented "blocking load gets the Wayland surface killed by the compositor" failure.

### ⚠️ Why the blob is wrapped, and why that is NOT optional

The specification says incompatible cache data is "ignored", but that promise is gated by
valid-usage rules (`VUID-VkPipelineCacheCreateInfo-initialDataSize-00768/-00769`): corrupt,
truncated or foreign bytes are **undefined behaviour**, and drivers do crash inside
`vkCreatePipelineCache`. DXVK abandoned driver-blob caching for exactly that reason.

So nothing reaches the driver before an application header matches in full: magic, format
version, blob size, FNV-1a content hash, `vendorID`, `deviceID`, `driverVersion`, pointer ABI
(`sizeof(void*)`) and the raw 16-byte `pipelineCacheUUID`. The last three are not redundant —
some drivers never bump their UUID on a breaking update, and a 32-bit and a 64-bit driver can
share one.

Three more defences, each answering a real failure mode:

- **A load marker.** One documented corruption originated *inside* `vkGetPipelineCacheData`, so
  the hash written at save time validated garbage. A `pipeline.cache.loading` file is dropped
  before `vkCreatePipelineCache` and removed after; finding it at startup means the previous run
  died in the driver's parser, and the blob is discarded.
- **Write-then-rename.** A `SIGKILL` — the documented fallback when `Core.shutdown()` is not used
  — must not leave a truncated blob for the next launch.
- **Zero-initialised destination** before `vkGetPipelineCacheData`: drivers leave the padding
  uninitialised, which makes the hash unstable and writes process memory into a file.

`--clear-renderer-cache` wipes it along with the other shader caches.

### Thread safety

`VkPipelineCache` is **internally synchronized by the specification** when passed to
`vkCreate*Pipelines`, so no external locking is needed for the current design. That guarantee is
lost if `VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT` is ever set, and
`vkMergePipelineCaches` always requires external synchronization of its destination.

> [!WARNING]
> `Device::createPipelineCache()` sets `VkPipelineCacheCreateInfo::flags = 0` **on purpose**, and
> that zero is load-bearing. `EXTERNALLY_SYNCHRONIZED_BIT` is not a free optimisation: it hands
> the locking duty back to the application, and **neither** `GraphicsPipeline::createOnHardware()`
> nor `ComputePipeline::createOnHardware()` takes any lock today — they call `vkCreate*Pipelines`
> straight through with `device()->pipelineCache()`. Setting that flag therefore means adding a
> mutex around **every** `vkCreate*Pipelines` call site in the same change. Skipping that step
> produces a data race inside the driver: no validation message, no crash site to blame.
