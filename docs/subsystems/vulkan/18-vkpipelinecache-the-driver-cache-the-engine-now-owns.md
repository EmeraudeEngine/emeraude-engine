## VkPipelineCache — the driver cache the engine now owns (Aug 2026)

`Vulkan::Device` owns the `VkPipelineCache` objects and PUBLISHES one of them through
`Device::pipelineCache()`, handed to both `vkCreateGraphicsPipelines` (`GraphicsPipeline.cpp`) and
`vkCreateComputePipelines` (`ComputePipeline.cpp`) — every pipeline in the engine goes through one of
those two. `Graphics::Renderer` does the disk I/O: `loadPipelineCache()` right after the device is
acquired (a cache must exist BEFORE any pipeline is created) and `savePipelineCache()` in
`onTerminate()` after `waitIdle()`. Since Oct 2026 the stored blob is restored on a worker
(§ "The asynchronous restore" below). Setting:
`Core/Graphics/Shader/EnablePipelineCache`, `DefaultPipelineCacheEnabled` = **`true`**
(`SettingKeys.hpp`) — this cache is **on by default**, and the Aug 2026 pass that flipped the
SPIR-V binary cache on left it untouched. On disk: `<cache>/pipeline-cache/pipeline.cache`
(+ the `pipeline.cache.loading` marker described below), via
`FileSystem::cacheDirectory("pipeline-cache")`.

`Device::pipelineCache()` returns `VK_NULL_HANDLE` when the setting is off or creation failed,
and `vkCreate*Pipelines` accepts that as "no cache" — which is why `GraphicsPipeline.cpp` and
`ComputePipeline.cpp` pass it unconditionally, with no null check and no second code path.

### The asynchronous restore — two caches, one published handle (Oct 2026)

Owner decision 2026-10-12 (P2 step 1 of the M6 diagnosis, engine `docs/todo/logic-stalls-on-runtime-pipeline-compile.md`;
step 2, pruning the cache, is `docs/todo/renderer-cache-pruning.md`). A driver
may compile the WHOLE blob inside `vkCreatePipelineCache`: MoltenVK on a cold Metal shader cache (after a
macOS / driver update, or a purge of `$(getconf DARWIN_USER_CACHE_DIR)<bundle id>`) compiled every Metal
library of the 40.7 MB blob there, on the MAIN thread, before any window content: a 113.6–114 s freeze
(M6, the earlier M2 measured 153 s).

- **Live cache**: `loadPipelineCache()` first creates an EMPTY cache (`Device::createPipelineCache()`, no
  compile) and publishes it. Every pipeline has a cache from the first frame on, whatever happens to the
  stored one.
- **Restored cache**: every cheap check stays on the main thread (setting, `--clear-renderer-cache`, the
  marker, the read, the header and hash). A valid blob goes to ONE `Base::Thread`
  (`Renderer::m_pipelineCacheRestore`): marker → `Device::restorePipelineCache(blob)` → marker erased →
  "Pipeline cache restored (N bytes) in T ms". On success the Device publishes the restored cache
  (release store; `pipelineCache()` is an acquire load). A caller reads the handle once per
  `vkCreate*Pipelines` and never keeps it; a handle read just before the publication stays valid, both
  caches live until `Device::destroy()`.
- **Atomics only, no mutex**: the live handle is written once before any pipeline exists, the restored one
  once by the restore, the published one by both (`NOTE` on the members in `Device.hpp`).
- **A pipeline requested before the restore ends** uses the live cache: a cold compile on its own thread,
  exactly like a run without a stored cache (owner D2: no coupling between scene loading and the restore).
- **Save**: with two caches, `Device::getPipelineCacheData()` creates a TEMPORARY cache, merges {restored,
  live} into it and reads it. The destination of `vkMergePipelineCaches` must be externally synchronized:
  only that call knows the temporary one, so a pipeline created meanwhile on another thread is no hazard.
- **Lifetime** (Ave Robustus II): the job captures values only (`shared_ptr` to the device, the blob, the
  marker path), no `this`. `onTerminate()` joins it BEFORE the save; its own destructor joins it too (an
  initialization failing after the start). `vkCreatePipelineCache` cannot be interrupted: a shutdown during
  the restore WAITS for it, traced "Waiting for the pipeline cache restore to finish" (owner D3).
  `Device::destroy()` during a restore is a contract fault: traced and aborted.
- **A refused thread start** (traced by `Base::Thread`) restores synchronously, the former behaviour.
  **A failed restore** keeps the live cache only; the next save writes its content.
- **The load marker spans the worker's call** (owner D4): a crash of the process during the restore window,
  related or not, discards the stored cache on the next launch (one cold run).

Measured on the Mac mini M6 (MoltenVK 1.4.2, engine cache 40 731 220 bytes, sponza, validation off for the
timings):

| Case | Before (synchronous) | After (worker) |
|---|---|---|
| Metal cache warm | synchronous on the main thread | restore 69–516 ms on the worker; scene loaded at 2.1 s |
| Metal cache COLD, engine cache warm | MAIN thread frozen 113.6–114 s, then the scene | scene loaded at **4.0 s**; console round trips 24–34 ms during the restore; restore **105.6 s** on the worker |
| same, sponza loaded during the restore | — | 128 cold compiles on render (14.77 s, max 196 ms), 15 on main at start-up (1.32 s), 2 on logic (0.22 s); logic max 229 ms |
| merge at save (MoltenVK) | — | 4–8 ms: the merge does NOT recompile |
| shutdown 31.6 s into a cold restore | — | waited 75 s for the driver, then merged and saved; marker erased |

Fault injection (temporary probes, reverted): a refused start (`Base::Thread::failNextStartsForTesting(1)`)
restores on the main thread (161 ms); a forced `VK_ERROR_INITIALIZATION_FAILED` keeps the live cache and
saves its content (4.8 MB, sponza only); a shutdown during a 25 s delayed restore waits, merges and saves;
a `SIGKILL` during a delayed restore leaves the marker, and the next launch says "The previous run crashed
while loading the pipeline cache ! Discarding it." and starts empty. 0 `VUID-` with validation on (sponza,
citadel).

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

Four more defences, each answering a real failure mode:

- **A load marker.** One documented corruption originated *inside* `vkGetPipelineCacheData`, so
  the hash written at save time validated garbage. A `pipeline.cache.loading` file is dropped
  before `vkCreatePipelineCache` and removed after; finding it at startup means the previous run
  died in the driver's parser, and the blob is discarded.
- **A SAVE marker, and a refusal** (2026-11-15, owner decision). The other side of the same call: one driver
  crashes while GIVING its data — an access violation inside the driver during the save at exit, right after
  "The restored and live pipeline caches merged", at EVERY exit of that machine (production: 4 reports from one
  machine, an Intel UHD Graphics 630 on an old driver; the session itself had worked). The engine cannot catch that,
  so it stops asking. `savePipelineCache()` drops `pipeline.cache.saving` before the merge and the read
  (`Device::getPipelineCacheData()`) and removes it once the driver returned. `loadPipelineCache()` finding it at the
  next launch says "The previous run crashed while saving the pipeline cache ! It is no longer saved with this
  driver." and writes `pipeline.cache.nosave`: 40 bytes naming the device and the driver (vendor, device, driver
  version, pointer width, cache UUID — the identity the cache file's own header carries). While that file names the
  device in use, the save is skipped with one line of information; when it names another one — the driver was
  updated — or cannot be read, it is erased and the save is tried again. `--clear-renderer-cache` erases it too.
  - The cache stored BEFORE stays good (written aside, then renamed) and is still restored: that machine keeps what
    it had, its cache only stops growing.
  - ⚠️ ANY death of the process inside the save leaves the marker, a kill included: the window is the merge and the
    read, a few milliseconds here, and the price of a false alarm is a cache which stops growing until the driver
    changes or the cache is cleared.
  - One crash per machine and driver instead of one per exit; no list of drivers to keep. "Save the cache earlier"
    was rejected: it would move a harmless death at exit into the session.
  - **Measured** (2026-11-15, Linux, RTX 3070 Ti, a clang build with fault injection, `default`, a scratch
    `--cache-directory`; `--inject-pipeline-cache-save-crash` aborts between the marker and the read): a first
    launch saves 1 438 758 bytes; a second restores, merges and saves; the third DIES in the save (SIGABRT) — the
    marker is there, the stored cache untouched; the fourth warns, writes the 40-byte refusal, restores the cache,
    does not save and exits 0; the fifth the same without the warning; with one byte of the refusal's driver
    version changed, the sixth erases it, merges and saves again; after another death, a launch with
    `--clear-renderer-cache` erases the marker and saves, and the next one is back to normal.
  - **Three OS** (peers' round 7, 2026-11-15, engine 4c8ee19f, a normal build, the marker file created BY HAND before a
    launch, six launches each): macOS (Apple M6, MoltenVK 1.4.2; 995 555 bytes) and Windows (RTX 3500 Ada, driver
    32.0.15.9658, MSVC 19.44; 1 399 328 bytes) — a first launch saves, a second restores, merges and saves; with the
    marker, the launch warns, restores, writes the 40-byte refusal (`53 4E 4D 45`, then the version, the vendor, the
    device, the driver, the pointer width and the cache UUID) and does not save, the stored cache untouched to the
    nanosecond; the next launch the same without the warning; `--clear-renderer-cache` erases the refusal and saves;
    the launch after is back to normal. Release and Debug 0 warning, the 40-byte `static_assert` holds on arm64 Apple
    clang and MSVC x64, 0 VUID, 0 `[Error]`, exit 0 everywhere.
- **Write-then-rename.** A `SIGKILL` — the documented fallback when `Core.shutdown()` is not used
  — must not leave a truncated blob for the next launch.
- **Zero-initialised destination** before `vkGetPipelineCacheData`: drivers leave the padding
  uninitialised, which makes the hash unstable and writes process memory into a file.

`--clear-renderer-cache` wipes it along with the other shader caches.

### Thread safety

`VkPipelineCache` is **internally synchronized by the specification** when passed to
`vkCreate*Pipelines`, so no external locking is needed for the current design. That guarantee is
lost if `VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT` is ever set, and
`vkMergePipelineCaches` always requires external synchronization of its destination — which is why
the save merges into a temporary cache only `getPipelineCacheData()` knows, never into a published one.

> [!WARNING]
> `Device::createPipelineCache()` and `restorePipelineCache()` set `VkPipelineCacheCreateInfo::flags = 0` **on purpose**, and
> that zero is load-bearing. `EXTERNALLY_SYNCHRONIZED_BIT` is not a free optimisation: it hands
> the locking duty back to the application, and **neither** `GraphicsPipeline::createOnHardware()`
> nor `ComputePipeline::createOnHardware()` takes any lock today — they call `vkCreate*Pipelines`
> straight through with `device()->pipelineCache()`. Setting that flag therefore means adding a
> mutex around **every** `vkCreate*Pipelines` call site in the same change. Skipping that step
> produces a data race inside the driver: no validation message, no crash site to blame.
