---
id: logic-stalls-on-runtime-pipeline-compile
title: Compile pipelines off the render-list walk (the render thread still stalls on a runtime compile)
status: open
priority: medium
scope: Vulkan (GraphicsPipeline, ComputePipeline, the pipeline cache), Graphics (program generation), Core (logic / render threads)
opened: 2026-10-03
tags: [macos, moltenvk, pipeline-cache, stall, threads]
---

# Compile pipelines off the render-list walk (the render thread still stalls on a runtime compile)

## Why

Seen by the macOS peer (M2, validation layers on), citadel, engine `cdcf68e7` and `669353ae`.
`logicsTask` cycles of 530 / 1988 ms in one launch, and 3324 / 2872 / 5257 / 2200 / 2010 / 2104 ms in another.
Some came after a teleport of the player, some after `PostProcess` commands or a screenshot; one capture timed
out. Never on Linux (today's citadel runs peak at 243 ms, the start-up load).

What the macOS hunt established (20 more launches, none reproduced):
- Both stalled launches are the ONLY ones whose Vulkan pipeline cache GREW: 23.97 → 24.72 MB and 30.16 →
  33.74 MB. Fewer shaders came from the binary cache there (603, 517 against 636–666). Every clean launch
  saved exactly what it restored.
- So the stalls are the FIRST launch after an engine change that alters shaders: runtime pipeline compiles.
  On MoltenVK each is SPIR-V → MSL, then a Metal library: hundreds of ms. The stall size follows the miss
  size: +0.75 MB gave 2.5 s of stalls, +3.58 MB gave 17.8 s.
- A fully COLD cache (`--cache-directory` empty) did not reproduce the mid-session stalls (one 1008 ms stall
  during the start-up load, BC7 recompression). The trigger may need a partially stale cache.
- Unexplained: why the LOGIC thread waits. During the sampled steps the logic thread was 6–7 % busy and the
  render thread 100 % (validation-slowed `vkQueueSubmit` encoding). A compile on the render thread, or a lock
  shared with the logic thread across it, would explain it. Not proven.

## What was done (2026-10-03)

Pipeline creations are now timed and asked whether the cache served them (`VkPipelineCreationFeedbackCreateInfo`,
Vulkan 1.3; `Vulkan/Utility.hpp`, `pipelineCreationFeedbackAvailable()`, `reportPipelineCreation()`). One taking
50 ms or more logs `Slow pipeline creation: '<pipeline>' took N ms on this thread (a pipeline cache HIT / MISS)`.
The Tracer's thread ID (always in the file log, in the console with `enableThreadInfos`) names the thread.
Linux proof: with the threshold at 0, 548 creations on a cold cache reported, all MISS with valid feedback,
0 `VUID-`. All took under 1 ms (NVIDIA's own disk shader cache); at 50 ms, a cold cache logs nothing.

## The cause, caught (macOS, 2026-10-03)

The trigger is MSL NEW TO THE MACHINE: macOS keeps a per-app Metal shader cache
(`$(getconf DARWIN_USER_CACHE_DIR)org.ln-isle.projet-alpha/{com.apple.metal, com.apple.metalfe, com.apple.gpuarchiver}`,
284 MB there). An engine-cache miss whose MSL that cache already holds compiles in under 50 ms. With those
directories moved aside for one launch, 15 logic stalls of 2.1–11.3 s and 446 slow creations (all MISS,
50–501 ms, 83.8 s in all).

The stacks (`sample`):
- LOGIC, 77 % blocked: `AbstractEntity::processLogics()` → `std::recursive_mutex::lock()` on `m_componentsMutex`.
- RENDER, 100 %: `Scene::populateRenderLists()` → `entity.forEachComponent(…)` → `checkRenderableInstanceForRendering()`
  → `getReadyForRender()` → `generateShaderProgram()` → `vkCreateGraphicsPipelines` → the Metal compile.

`forEachComponent()` holds the entity's components lock for the whole callback, so the render thread held it across
every compile. The logic thread waited entity after entity.

**Fixed (owner: prepare outside the lock).** `populateRenderLists()` (raster and RT walks), the shadow-caster walk
and `forEachRenderableInstance()` (it held the scene-wide node and static-entity locks across
`initializeRenderTarget()`'s preparations) now COPY the renderable instances (`shared_ptr`) under the locks and
prepare them with none held. Linux proof, a PROOF-TEMP 150 ms delay in every graphics pipeline creation, cold cache:
532 slow creations (≈ 80 s of compile on the render thread), logic max 45 ms, 0 cycles over 500 ms, 0 `VUID-`.

The report was completed the same day. It names the pipeline by a label kept in Release (the generator's pass, e.g.
`RenderableInstanceSpotLightPassFull`; an effect's own pipeline is "unlabelled"), and the thread by its native ID
(`Tracer::currentThreadID()`; macOS printed `-1` before). The caller's location is passed through.

A second finding: with the Metal cache cold, MoltenVK's `vkCreatePipelineCache` eagerly compiles every shader
library of the restored engine cache, synchronously, on the MAIN thread at init (`Renderer::loadPipelineCache` →
`MVKPipelineCache::readData` → `MVKMetalCompiler::compile`). A 153 s start there. A bigger engine pipeline cache means
a slower cold start on macOS: relevant to the cache's size and trim policy.

**Accepted on macOS M2** (engine `e743b244`, the catch scenario: a cold Metal cache, the engine pipeline cache and
shader binaries removed, validation on). The miss workload was the same as the catch: 446 slow creations, all MISS,
82.5 s against 83.8 s. `logicsTask` over 500 ms: 0 against 15; the max is 278 ms against 11 324 ms. 0 `VUID-`. The
"on thread" IDs match `sample`'s exactly: 422 creations on the render thread, 22 on the main thread at start-up,
and 2 on the LOGIC thread (below).

## What remains

- The RENDER thread still stalls on a runtime compile: a missed frame per slow pipeline, seconds in all on a cold
  Metal cache. Cure, with the owner: compile asynchronously (a background queue, the instance skipped or drawn with
  a fallback until ready), and/or warm the pipelines at load.
- The cold-start compile of the restored pipeline cache on macOS (above).
- Two COMPUTE pipelines are created on the LOGIC thread itself (macOS, 124 and 144 ms, unlabelled, right after
  "Scene will use environment cubemap …": most likely the IBL baker's). A cold miss there lands on the logic.
  Move that creation off the logic thread, or pre-create the baker's pipelines at start-up.
- Label the effects' and compute pipelines too (46 "an unlabelled pipeline" lines on macOS).
