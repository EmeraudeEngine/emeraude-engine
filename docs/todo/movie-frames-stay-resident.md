---
id: movie-frames-stay-resident
title: Host RAM — every movie resource holds all its frames decoded as pixmaps
status: open
priority: unranked
scope: Graphics/MovieResource, Graphics/CubemapMovieResource, animated textures
opened: 2026-10-03
tags: [memory, textures]
---

# Host RAM — every movie resource holds all its frames decoded as pixmaps

## Why

`MovieResource` keeps every frame as an uncompressed `Pixmap` (`m_frames`, with its duration) for the resource's whole
life. The memory census (`Core.ResourcesManagerService.memoryCensus()`, 2026-10-03, Linux) counts **492 MiB for 7
movies in citadel** (terrain the same; forest 12 MiB for 6). `CubemapMovieResource` follows the same pattern (six faces
per frame).

Split from `cpu-copies-retained-after-upload` by the owner (2026-10-03): a different mechanism from the release /
reload of GPU-uploaded copies.

**The frames were held TWICE — FIXED 2026-10-04 for `MovieResource`** (owner: "image or owned pixmap"): a
`MovieResource::Frame` now either SHOWS a store `ImageResource` (kept alive, never copied) or OWNS its pixels (the
generated frames: debug, noise, parametric, `load(frames)`, the WAD flats); `Frame::pixmap()` answers either, and
`memoryOccupied()` counts only the owned bytes (the shared ones are the images' container's). Citadel census
2690 → 2198 MiB (movies 492 → 0, images held by the store alone 508 → 16 MiB), RSS at rest 5855 → 5515 MiB,
0 VUID. ⚠️ `CubemapMovieResource` still COPIES its frames out of store cubemaps (`m_frames.emplace_back(
cubemapResource->faces(), …)`, `CubemapMovieResource.cpp` load paths): same fix to do (none loaded in citadel).

What stays open here is the residency itself: every frame decoded, for the movie's whole life.

## What remains

1. Find who reads the frames after the animated texture's upload (the frame count and durations are read every frame
   by `BindlessTextureManager::syncTextureSet` / `frameIndexAt`: they must survive as metadata) and whether the GPU
   holds every frame already (an array texture), which would make the CPU frames a pure duplicate.
2. State of the art first: block-compressed frames (BCn / KTX2 array), or streamed decode into a small ring.
3. Owner decision on the approach.

## References

- `src/Graphics/MovieResource.hpp`, `src/Graphics/CubemapMovieResource.hpp`,
  `docs/todo/cpu-copies-retained-after-upload.md` § Phase 0.
