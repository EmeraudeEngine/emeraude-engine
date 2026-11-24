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

## Reproduced on Linux (2026-11-24, RTX 3070 Ti, driver 620.70.06, engine `develop` 74beadf5)

`city`, window-less, validation off, one console call (`Camera.getActive()`) every half second from the launch, each
given 2 s to answer. The cold case: a COPY of the owner's settings with `Core/Graphics/Shader/EnablePipelineCache`
false (the engine then neither reads nor writes the cache file: the owner's was checked untouched) and the driver's
own shader cache off for that process (`__GL_SHADER_DISK_CACHE=0`).

| | "The scene 'city' successfully loaded" | first console answer | console silent after the loaded line | slow pipeline creations |
|---|---|---|---|---|
| cold, run 1 | t = 8.1 s | t = 20.6 s | 12.5 s | 33 (2.1 s in all; 30 on one thread, 1.75 s) |
| cold, run 2 | t = 8.1 s | t = 20.1 s | 12.0 s | 34 |
| warm, first launch (the cache did not hold this scene yet) | t = 8.1 s | t = 13.1 s | 5.0 s | 18 |
| warm, second launch | t = 8.1 s | t = 8.1 s | none | 0 |

So the console IS starved after the scene reports itself loaded, on a fast discrete GPU too, in proportion to how
cold the cache is; once it answers, it answers in 5 to 27 ms. ⚠️ The creations reported slow add up to 2 s of the
12: most of the silence is not in them (creations under the report's threshold, or other first-use work — not
attributed). Nearly all the slow ones are on ONE thread here as on Windows.

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
