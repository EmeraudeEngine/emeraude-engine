---
id: renderer-cache-pruning
title: Prune the on-disk renderer caches automatically
status: open
priority: medium
scope: Graphics/Renderer, Vulkan/Device (the pipeline cache)
opened: unknown
tags: [cache, housekeeping]
---

# Prune the on-disk renderer caches automatically

## Why

Nothing cleans the cache directories except `--clear-renderer-cache` (which since Aug 2026 clears
ALL THREE on-disk renderer caches: the SPIR-V binary cache, the `VkPipelineCache` and the texture
cache). The engine is pre-release, so this is a structural gap, not a backlog of user data.

Two shapes of it, both of which pruning must cover:

1. **A directory that stops being written to** — the generated-GLSL dump was renamed
   `shader-sources/` → `generated-shaders/`.
2. **Entries whose FILENAME stops being produced** because a content-addressed key scheme changed
   — the texture-cache key fix orphaned 40 entries here. Orphaned, not invalidated: nothing ever
   opens them, so no header check can catch them.

Both were cleared by passing the switch by hand, which is exactly what a mechanism should make
unnecessary.

## What remains

- [ ] **One** mechanism for every `~/.cache/<app>/` renderer directory (age / total size / stale
  format version), not one per cache.
- [ ] The `VkPipelineCache` too — next section (its own constraints, to bring back to the owner).

## The pipeline cache (P2 step 2 of the M6 diagnosis, folded in 2026-10-13)

Why it matters beyond disk space: MoltenVK compiles EVERY Metal library of the restored blob inside
`vkCreatePipelineCache` when the machine's Metal shader cache is cold (M6: 113.6–114 s for the 40.7 MB blob; the M2
measured 153 s). Step 1 (engine `d58de797`, 2026-10-12) moved that restore to a worker
(`docs/subsystems/vulkan/18-vkpipelinecache-the-driver-cache-the-engine-now-owns.md` § "The asynchronous restore"):
the main thread no longer waits, but the worker still pays for every demo's pipelines. Data point: sponza alone saves
a 4.8 MB cache; the all-demo cache is 40.7 MB — sponza pays for citadel's.

The blob is OPAQUE: one `VkPipelineCache` cannot be trimmed entry by entry, so pruning needs several caches keyed by
what the engine knows. Options, back to the owner with the step-1 numbers:
- (a) a size / age cap at save (the whole blob dropped past it);
- (b) one cache per scene, restored behind its loading screen (shared pipelines duplicated) — the macOS peer's leaning;
- (c) one blob per pipeline keyed by the engine's hash, LRU (on MoltenVK a per-pipeline restore on a cold Metal cache
  costs about a cold compile anyway).

The render-thread side of the same diagnosis is `logic-stalls-on-runtime-pipeline-compile` (P1); the other two cures
are `ibl-compute-pipelines-created-on-the-logic-thread` (P3) and `label-effect-and-compute-pipelines` (P4).

## Note — not an item

`SpvOptions::disableOptimizer` in `ShaderManager.cpp`: glslang is built here with `ENABLE_OPT=OFF`
(`libSPIRV.a` contains zero SPIRV-Tools symbols), so the flag is SILENTLY IGNORED. Honouring it
would mean adding SPIRV-Tools to the dependency cascade for no gain — desktop NVIDIA/AMD drivers
re-optimise the SPIR-V they receive anyway.
