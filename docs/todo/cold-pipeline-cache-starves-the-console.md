---
id: cold-pipeline-cache-starves-the-console
title: With a cold pipeline cache, the console stops answering for more than 30 s after the scene loads
status: open
priority: unranked
scope: Vulkan pipeline creation (the pipeline cache), the console / remote listener while pipelines are created
opened: 2026-11-02
tags: [pipeline-cache, console, startup]
---

# With a cold pipeline cache, the console stops answering for more than 30 s after the scene loads

## Why

Windows-PA, 2026-11-01, Intel Raptor Lake iGPU, `city`, the first launch on that device after a build, so the pipeline
cache was cold:
- the journal shows 189 "Slow pipeline creation … (a pipeline cache MISS)" lines at about 240 ms each, 187 of them on
  ONE thread (≈ 45 s of serial creation);
- after "The scene 'city' successfully loaded", `Camera.getActive()` got no console answer within 30 s;
- `Core.shutdown()` then exited cleanly, and a relaunch with the warm cache had 0 slow pipeline.

Two questions: why are almost all the creations on one thread, and why does that starve the console (and probably
the frame) after the scene reports itself loaded?

## What remains

- Reproduce with a deliberately cold cache on Linux and Windows (a scratch cache directory). Time the console's
  answers and the frames during the creations.
- Attribute the single thread: deferred creations on first use (the render thread), or a serialized compile queue.
- Owner decision once attributed: create on the loading threads before the scene is declared loaded, spread over
  workers, or accept and document.

## ⚠️ Traps

- Never kill an instance during the pipeline-cache restore: the crash guard then discards the cache (projet-alpha
  `test-instance` skill).

## References

- Windows-PA's report of 2026-11-01, round 2 of the TAA large-change test (graphics doc 13-12 § The TAA resolve).
