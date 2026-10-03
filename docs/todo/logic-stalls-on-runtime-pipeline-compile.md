---
id: logic-stalls-on-runtime-pipeline-compile
title: Multi-second logic stalls on macOS after a shader change (runtime pipeline compiles)
status: open
priority: unranked
scope: Vulkan (GraphicsPipeline, ComputePipeline, the pipeline cache), Graphics (program generation), Core (logic / render threads)
opened: 2026-10-03
tags: [macos, moltenvk, pipeline-cache, stall, threads]
---

# Multi-second logic stalls on macOS after a shader change (runtime pipeline compiles)

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

## What remains

- Catch one stall with the report on: on macOS, warm the cache with an older engine (e.g. `dccc0329`), then the
  first citadel launch of a newer one whose shaders differ, under `sample`. The report names which pipelines
  compiled and on which thread; the stacks say what the logic thread waits on.
- Then decide the cure with the owner: compile pipelines off the critical threads (asynchronously, with a
  fallback draw or a skipped draw meanwhile), warm the cache at load, or release the lock the logic waits on.
