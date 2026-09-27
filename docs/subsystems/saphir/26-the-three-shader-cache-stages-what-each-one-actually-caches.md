## The three shader-cache stages — what each one actually caches (audited Aug 2026)

Three DIFFERENT things are persisted to disk, under three separate setting keys. They are not
tiers of one mechanism, and only two of them are caches at all:

| Stage | Setting key | Default | Stores | Read back? | Measured gain |
|---|---|---|---|---|---|
| 1. Source dump | `Core/Graphics/Shader/EnableSourceCodeDump` | `false` | the generated GLSL, one sub-directory per generator | **NEVER** | none — it is an inspection tool |
| 2. SPIR-V binary cache | `Core/Graphics/Shader/EnableBinaryCache` | **`true`** since Aug 2026 | glslang's SPIR-V output, one blob per shader | yes | 393 ms → 10.3 ms on 232 modules (**38x**) |
| 3. `VkPipelineCache` | `Core/Graphics/Shader/EnablePipelineCache` | `true` | the driver's SPIR-V→ISA result, one blob | yes | 5702 ms → 31 ms on 294 pipelines (**182x**) |

The defaults live in `SettingKeys.hpp` (`DefaultSourceCodeDumpEnabled`,
`DefaultBinaryCacheEnabled`, `DefaultPipelineCacheEnabled`).

`--clear-renderer-cache` wipes the shader caches, the pipeline cache included.

### Stage 1 — the generated-GLSL DUMP, not a cache (renamed Aug 2026)

`Core/Graphics/Shader/EnableSourceCodeDump` writes every generated GLSL to
`~/.cache/<app>/generated-shaders/`. **Nothing ever reads it back**:
`AbstractShader::loadSourceCode()` — the only function able to re-inject a file into a shader —
has zero callers in the whole cascade. It structurally cannot be a cache either: its key is
`std::hash` of the source it stores, so you must have generated the source already to know which
file to read. That is why it was renamed away from "cache" — the old name described something
this facility never was.

> [!WARNING]
> **The Aug 2026 rename ships with NO migration, deliberately.** No alias, no fallback, no
> warning at startup — `Settings` has no key-migration mechanism at all. Consequence: an
> existing `settings.json` keeps `Core/Graphics/Shader/EnableSourceCodeCache` as **dead JSON**
> that is silently ignored, and anyone who had the dump enabled finds it **OFF** until they set
> `Core/Graphics/Shader/EnableSourceCodeDump`. The owner accepted the break because this is a
> debug facility defaulting to `false`. Do NOT add a compatibility alias to "fix" a report of
> the dump having stopped — point at the new key instead.
>
> The private `ShaderManager` symbols moved with it (no public API touched):
> `SourceCodeCacheEnabledKey`→`SourceCodeDumpEnabledKey`,
> `DefaultSourceCodeCacheEnabled`→`DefaultSourceCodeDumpEnabled`,
> `m_sourceCodeCacheEnabled`→`m_sourceCodeDumpEnabled`,
> `ShaderSourcesDirectoryName`→`GeneratedShadersDirectoryName`,
> `m_shadersSourcesDirectory`→`m_generatedShadersDirectory`,
> `cacheShaderSourceCode()`→`dumpShaderSourceCode()`,
> `generateShaderSourceCacheFilepath()`→`generateShaderDumpFilepath()`,
> `readCache()`→`readBinaryCache()` (it only ever reads binaries now). The two sibling keys
> `EnableBinaryCache` and `EnablePipelineCache` are untouched, and so is `ShowSourceCode` —
> that one logs, it does not write files.

It exists to let a human inspect what the generators produced, and it is now shaped for that:

- **one sub-directory per generator** (`SceneRendering/`, `ShadowCasting/`, `PostProcessing/`,
  `OverlayRendering/`, `GizmoRendering/`, `TBNSpaceRendering/`), created lazily. The generator
  identity is carried by `Generator::Abstract::generatorClassId()` and threaded through
  `ShaderManager::getShaderModules()`;
- the dump now happens **before** the binary-cache check. It used to hang off the compile path,
  so a binary cache hit silently stopped producing it.

Re-verified at runtime after the rename (`material-debug`, all 10 options): the dump lands in
`generated-shaders/` — 232 files, SceneRendering 226, OverlayRendering 3, PostProcessing 2,
ShadowCasting 1 — and the run logs no error. ⚠️ That file count and the 336 below are **different
measurements**, not a contradiction: 336 counts distinct generated SceneRendering sources, not
files left on disk. Do not try to reconcile them.

⚠️ Measured on one `material-debug` load: **336 distinct sources for SceneRendering alone**
(265 fragment, 71 vertex), against 3 for OverlayRendering, 2 for PostProcessing and 1 for
ShadowCasting. That number is the program-variant count, and it is the real load-time driver —
worth understanding before optimising any cache.

⚠️ **`readBinaryCache()` no longer touches this directory at all** — hence its name: it only ever
reads binaries now. It used to index the dumped sources into `m_cachedShaderSourceCodes` — a
member that was written and never once read, not even by `clearCache()`, which walks the
directory itself. Since `readBinaryCache()` only runs when the *binary* cache is on, and the dump
is off by default, that loop was scanning an **empty path** and logging an
`IO::directoryEntries()` error on every startup as soon as the binary cache became the default.
The member is gone and the function returns early on an empty binaries directory. `clearCache()`
guards both of its loops for the same reason: a disabled facility leaves its path empty, and
`--clear-renderer-cache` runs whatever the settings say. See
[`docs/caution-points.md`](../../caution-points.md) § "Flipping a default to ON runs a path
nobody had ever run".

### Stage 2 — the binary (SPIR-V) cache — hardened, and ON BY DEFAULT since Aug 2026

`Core/Graphics/Shader/EnableBinaryCache` skips glslang on a hit. `DefaultBinaryCacheEnabled`
flipped from `false` to **`true`**. What paid for the flip, measured 2026-08-13 on
`material-debug` with all 10 options (RTX 3070 Ti, Release). The instrumented envelope is
source dump + cache lookup + glslang compile + `vkCreateShaderModule`, placed **after** the
in-memory ShaderModule hash lookup, so it counts cache MISSES only — **232 shader modules**:

| Run | Total | Per module |
|---|---|---|
| cache OFF | 393 ms | 1.69 ms |
| cache ON, cold (writes the 232 blobs) | 391 ms | 1.68 ms |
| cache ON, warm (reads) | **10.3 ms** | **0.044 ms** |

**38x faster, 383 ms saved — and writing the cache on a cold run is FREE** (391 vs 393 ms is
noise). That absence of a first-launch penalty is the whole argument for the default: there is
nothing to trade away. 0 residual `.tmp` files.

What made it safe enough to enable (engine commit `56fabc9a`): the filename says WHICH shader
(`<name>_<source hash>.bin`); an application header now says
whether the blob is still VALID, and **every field is checked before a byte reaches
`vkCreateShaderModule`**: magic, format version, source hash, shader stage, blob size, FNV-1a
content hash, and a **toolchain identity hash** — glslang's version string, its SPIR-V generator
version, the client/target environment pair (⚠️ macOS targets Vulkan 1.2 / SPIR-V 1.5, everything
else 1.3 / 1.6) and the engine version. A rejected file is deleted and the shader recompiled.

⚠️ That toolchain hash is the whole point: without it a glslang upgrade left stale SPIR-V on disk
and it was fed to the driver unchecked. Plus two structural checks the blob must pass anyway —
size a multiple of 4, and the SPIR-V magic word `0x07230203` as its first word.

Writes go to a `.tmp` file and are renamed, so a `SIGKILL` cannot leave a truncated blob for the
next launch.

Verified live during that hardening pass — a **separate run** from the timings above, hence a
different blob count: 342 blobs written on the first run and all 342 reloaded on the second, with
no stray `.tmp`; a blob corrupted in its data and another with a falsified toolchain hash were
both rejected and recompiled — 2 rejected, 340 reused, no crash.

### Stage 3 — the `VkPipelineCache` — it EXISTS now (engine commit `e583df40`)

⚠️ This file used to state "There is NO VkPipelineCache". **That is obsolete** — do not act on
that claim if you find it echoed elsewhere.

`Core/Graphics/Shader/EnablePipelineCache`, default `true` (already `true` before the Aug 2026
binary-cache pass, unchanged by it). This is the stage that matters most, because it skips the
DRIVER's SPIR-V→ISA compilation. Measured on the same demo, 294 graphics pipelines:

| Run | Total |
|---|---|
| driver cache active | 33 ms |
| driver cache OFF | 5702 ms |
| driver cache OFF, engine cache restored from disk | 31 ms |

**182x.** The serialised blob is 7.4 MB.

Ownership split, do not move it: `Vulkan::Device` owns the `VkPipelineCache` object,
`Graphics::Renderer` does the disk I/O (`loadPipelineCache()` / `savePipelineCache()`).

### glslang's optimizer is COMPILED OUT of this build

`ShaderManager.cpp` sets `SpvOptions::disableOptimizer = true` before `GlslangToSpv()`.
**That flag is SILENTLY IGNORED.** glslang is built here with `ENABLE_OPT=OFF`: `libSPIRV.a`
contains ZERO SPIRV-Tools symbols (verified 2026-08-13), so there is no optimizer to disable in
the first place — and nothing to enable either.

Do not chase that flag for compile time or SPIR-V quality. Turning a real optimizer on would
mean adding SPIRV-Tools to the dependency cascade for no gain: desktop NVIDIA/AMD drivers fully
re-optimize whatever SPIR-V they receive. The levers that actually move load time are the two
caches above and the number of program VARIANTS (336 distinct SceneRendering sources on a single
`material-debug` load).
