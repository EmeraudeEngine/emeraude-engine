---
id: movie-load-ignores-the-stop
title: A movie resource's frame-by-frame load and compression ignores the stop, so a shutdown waits for it (46 s)
status: in-progress
priority: high
scope: src/Graphics/MovieResource.cpp, Resources (the 'Movie manager' container), TextureCompressorService
opened: 2026-10-24
tags: [ave-robustus-ii, shutdown, cancellation, defect, measured]
---

# A movie resource's frame-by-frame load and compression ignores the stop, so a shutdown waits for it (46 s)

## Why

Measured by macOS-PA on 2026-10-24 (Mac mini M6, engine `7b453edb`). `water-world` was launched with an empty
`--cache-directory`, so the compressed textures were not cached, and `Core.shutdown()` was sent ~2 s after the act
activated. The shutdown took **46.5 s**: `stopBackgroundWork(): the 'Movie manager' tasks took 46289 ms to stop`.
The `WaterNormal` `MovieResource` was being compressed frame by frame (`TextureCompressorService Compressed
1024x1024 texture to BC5 (11 mip levels, 1365 KB)`): 346 frames in the run, **71 of them after the shutdown
command**. Windows-PA saw the same the same day (RTX 3500 Ada): ~70 s to exit after `Core.shutdown()` on
water-world's first launch with an empty cache, 345 `TextureCompressorService` lines. The log then said `The resource (MovieResource) 'WaterNormal' is loaded` before the manager
terminated. The exit itself was clean (EXIT 0, 0 VUID, 0 `[Error]`).

So the movie's load never checks the stop between two frames. Ave Robustus II requires a background task to stop
within a bounded latency through its `std::stop_token`.

## What remains

- Done on Linux (2026-11-01, owner decision: the explicit token, narrow form): the container's task token reaches the
  completion chain (`ResourceTrait::LoadStopScope`, `onDependenciesLoaded(stopToken)`, `createTexture(renderer,
  stopToken)`); `AnimatedTexture2D` encodes its frames in parallel and checks the stop before each one. Contract and
  measurements: `docs/subsystems/resources/06-async-lambda-capture-safety.md` § The stop reaches the completion chain
  (`water-world` cold, 4 cores, shutdown 2 s after load: 111.4 s → 7.6 s, the 'Movie manager' tasks 106 s → 2.8 s).
- [ ] The peers: the same repro on macOS (`--cache-directory` empty) and Windows; the base suites; then delete this item.

## ⚠️ Traps

- `--cache-directory` did not reset the PIPELINE cache on macOS (it lives elsewhere): an empty directory gives cold
  texture compression, not cold pipelines.

## References

- `cancellation-through-stop-token` (the same rule elsewhere: SharingServer, PeerStore, UDPClient),
  `movie-frames-stay-resident` (the movie's RAM, a different defect).
- `.claude/rules/ave-robustus-ii.md`; emeraude-base `Base::TaskHandle` / `TaskGroup`.
