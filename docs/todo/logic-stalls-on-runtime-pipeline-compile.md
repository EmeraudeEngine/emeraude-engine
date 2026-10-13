---
id: logic-stalls-on-runtime-pipeline-compile
title: The render thread stalls on every pipeline compiled at first use (P1, warm the scene's pipelines in parallel at load)
status: open
priority: medium
scope: Graphics (program generation, Scene render-list walk), Vulkan (GraphicsPipeline, the pipeline cache), Core (render thread, scene loading)
opened: 2026-10-03
tags: [macos, moltenvk, pipeline-cache, stall, threads]
---

# The render thread stalls on every pipeline compiled at first use (P1, warm the scene's pipelines in parallel at load)

## Why

A pipeline is created when an instance is first prepared for rendering, inside the render thread's walk:
`Scene::populateRenderLists()` → `checkRenderableInstanceForRendering()` → `getReadyForRender()` →
`generateShaderProgram()` → `vkCreateGraphicsPipelines`. A slow compile there costs one missed frame per pipeline.
On NVIDIA (Linux) every creation takes under 1 ms (its own disk shader cache); on MoltenVK each one is SPIR-V → MSL,
then a Metal library: 50–500 ms when the MSL is new to the machine's Metal shader cache
(`$(getconf DARWIN_USER_CACHE_DIR)org.ln-isle.projet-alpha/{com.apple.metal, com.apple.metalfe, com.apple.gpuarchiver}`).

Measured on the Mac mini M6 (MoltenVK 1.4.2, 2026-10-12, validation off; one fully cold run with it gave the same
numbers, 0 `VUID-`), fully cold (Metal cache and engine pipeline cache removed): the render thread takes every compile,
one missed frame each (50–311 ms) — sponza 132 misses / 14.1 s, citadel 264 / 37.1 s (mostly the forward light-pass
permutations, one set per material, 17 materials there). The same happens during a cold pipeline-cache restore: a
scene loaded while it runs compiles on render (owner D2; M6: sponza 128 cold compiles on render, 14.77 s, max 196 ms).
`MVK_CONFIG_SHOULD_MAXIMIZE_CONCURRENT_COMPILATION` changes nothing: the compiles run one at a time on the calling
thread.

## History (what P1 builds on)

- **2026-10-03, macOS M2 (citadel)**: `logicsTask` cycles of 2.1–11.3 s on the first launch with MSL new to the Metal
  cache. Cause: `AbstractEntity::forEachComponent()` held the entity's components lock across the whole walk, and the
  render thread compiled inside it; the logic thread waited on the same lock in `processLogics()`. **Fixed** (owner:
  prepare outside the lock): the raster, RT and shadow-caster walks and `forEachRenderableInstance()` copy the
  instances (`shared_ptr`) under the locks and prepare them with none held. Accepted on the M2 (engine `e743b244`):
  446 slow creations (82.5 s, all MISS), 0 `logicsTask` over 500 ms against 15, max 278 ms against 11 324 ms, 0 `VUID-`.
  Rule and story: `docs/caution-points.md` § "Fixed: the render thread held an entity's components lock across a
  pipeline compile". The M6 run of 2026-10-12 confirms it holds (logic max 258 ms, 0 cycle over 500 ms).
- **The measuring tool** (2026-10-03): every pipeline creation is timed and asks the cache whether it served it
  (`VkPipelineCreationFeedbackCreateInfo`, Vulkan 1.3; `Vulkan/Utility.hpp`, `pipelineCreationFeedbackAvailable()`,
  `reportPipelineCreation()`). One taking 50 ms or more logs `Slow pipeline creation: '<label>' took N ms on thread
  <native id> (a pipeline cache HIT / MISS)`. The label is the generator's pass (e.g.
  `RenderableInstanceSpotLightPassFull`). Linux proof: threshold 0, 548 creations on a cold cache, all MISS with valid
  feedback, all under 1 ms. On the M2 the thread IDs matched `sample`'s: 422 creations on render, 22 on main at
  start-up, 2 on the logic thread (item `ibl-compute-pipelines-created-on-the-logic-thread`).
- **P2 step 1 done** (engine `d58de797`, 2026-10-12): the pipeline cache is restored on a worker, the main thread no
  longer freezes 114 s in `vkCreatePipelineCache` on a cold Metal cache — `docs/subsystems/vulkan/18-vkpipelinecache-the-driver-cache-the-engine-now-owns.md`
  § "The asynchronous restore".

The M6 diagnosis proposed four cures (P1-P4); the owner chose P2 first. They are one item each now:
P1 here; P2 step 2 (pruning the pipeline cache) in `renderer-cache-pruning`; P3
`ibl-compute-pipelines-created-on-the-logic-thread`; P4 `label-effect-and-compute-pipelines`.

## What remains

- **Design with the owner** (not started): warm the scene's pipelines IN PARALLEL at load, behind the loading screen
  (the thread pool), so the render thread finds them ready; as the fallback, an instance whose pipeline is missing at
  first use gets it built ASYNCHRONOUSLY and is skipped until ready, never compiled inline on the render thread.
- What the warm-up must enumerate: every program an instance of the scene will request (each render pass type and
  light-pass permutation per material and layer), the same keys `generateShaderProgram()` builds — no second list.
- Proof: the M6 cold runs above (sponza, citadel) with 0 missed frame over 50 ms from compiles on the render thread,
  and the same on a cold restore; Linux and Windows unchanged; 0 `VUID-`.
- It would cure `imposter-bake-lit-rig-compiles-on-render-thread` too (the lit bake rig's synchronous compiles).

## ⚠️ Traps

- Linux hides the problem: NVIDIA compiles in under 1 ms. Reproduce on macOS with the Metal cache moved aside (the
  directories above), or on Linux with a temporary delay in every pipeline creation (the 2026-10-03 proof used 150 ms).
- An engine-cache MISS is harmless when the Metal cache already holds the MSL (under 50 ms on the M6): a cold Metal cache
  is the condition, not a cold engine cache.
- Never hold a lock the logic thread takes across a preparation that may generate a program or create a pipeline
  (the 2026-10-03 rule).

## References

- engine `docs/caution-points.md` § "Fixed: the render thread held an entity's components lock across a pipeline compile".
- engine `docs/subsystems/vulkan/18-vkpipelinecache-the-driver-cache-the-engine-now-owns.md`.
- Items `renderer-cache-pruning` (P2 step 2), `ibl-compute-pipelines-created-on-the-logic-thread` (P3),
  `label-effect-and-compute-pipelines` (P4), `imposter-bake-lit-rig-compiles-on-render-thread`.
