---
id: renderer-cache-pruning
title: Prune the on-disk renderer caches automatically
status: open
priority: medium
scope: Graphics/Renderer
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

## Note — not an item

`SpvOptions::disableOptimizer` in `ShaderManager.cpp`: glslang is built here with `ENABLE_OPT=OFF`
(`libSPIRV.a` contains zero SPIRV-Tools symbols), so the flag is SILENTLY IGNORED. Honouring it
would mean adding SPIRV-Tools to the dependency cascade for no gain — desktop NVIDIA/AMD drivers
re-optimise the SPIR-V they receive anyway.
