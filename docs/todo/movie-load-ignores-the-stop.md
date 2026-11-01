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
- [x] AMD (LinuxB-PA, RADV Renoir, 2026-11-01): `city` cold, the ocean's material 2.9 s after the scene (116.9 s
      before); `water-world` cold, shutdown 2 s after load → exit 4.72 s, the 'Movie manager' tasks 224 ms; 0 VUID.
- [x] macOS (M6, AppleClang 21, 2026-11-01): `water-world` cold, shutdown 2 s after load → exit 3.75 s (46.5 s on
      10-24), the 'Movie manager' tasks 662 ms; 271 TUs `-Wdocumentation -Werror` clean; 0 VUID. The frames are still
      encoded during the ~2.9 s of the shutdown that come BEFORE `stopBackgroundWork()` (the logic and render threads
      join first): the token is requested there, not at the shutdown command — the "proceed stop at the shutdown request"
      the owner did not choose on 2026-10-15 (item `act-removal-stops-its-pending-loads`). With 0 (`water-world` option
      0) the lagoon's material is `BuiltinLagoonMaterial`, not `BuiltinOceanMaterial`: cold, it loads 12.9 s after the
      scene on the M6 (124 BC5 frames).
- [x] Windows (RTX 3500 Ada, MSVC /W4 /WX, 2026-11-01): 03b92fb7 did not LINK (`ResourceTrait::LoadStopScope`, a nested
      class, was not exported: fixed by `EMEN_LEAN_API` on it, docs/windows-export-api.md § 3); with the fix 0 warning,
      0 VUID. `water-world` cold, shutdown → exit 18.44 s: the 'Image manager' tasks took 13564 ms, the 'Movie manager'
      485 ms AFTER them — all 120 frames encoded anyway (below).
- [x] **The stop order** (owner decision 2026-11-01: request all, then wait all; done the same day, resources doc 06 —
      Windows to re-measure): `Manager::stopBackgroundWork()` stops
      the containers one after another (request + wait each) in `unordered_map` order — on MSVC the Image manager comes
      first and waits for its tasks queued behind the ocean's frame encodings on the shared pool, so the Movie manager's
      token (which stops those encodings) is requested only once they are ~96 % done. Split the container's stop
      (request, then wait) and request every container's stop before waiting for any.
- [ ] The REST of the stop is the Image manager (same AMD run: 3744 ms): a single `Texture2D` BC5 encoding has no stop
      check inside (one 2048² normal map ≈ 1.75 s on that CPU), and four plain normal maps were still encoded after the
      command — whether the last one STARTED after the stop request is not provable from the log (the encode line names
      no time of start). To decide with the owner: a stop token down to `TextureCompressor::compress()` (checked per mip
      or per block row), and/or a check that no encoding starts once the stop is requested.
- [ ] The peers: the same repro on macOS (`--cache-directory` empty) and Windows; then close this item (its knowledge in
      resources doc 06).

## ⚠️ Traps

- `--cache-directory` did not reset the PIPELINE cache on macOS (it lives elsewhere): an empty directory gives cold
  texture compression, not cold pipelines.

## References

- `cancellation-through-stop-token` (the same rule elsewhere: SharingServer, PeerStore, UDPClient),
  `movie-frames-stay-resident` (the movie's RAM, a different defect).
- `.claude/rules/ave-robustus-ii.md`; emeraude-base `Base::TaskHandle` / `TaskGroup`.
