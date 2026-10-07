---
id: movie-frames-stay-resident
title: Host RAM — every movie resource holds all its frames decoded as pixmaps
status: open
priority: medium
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

Split from the CPU-copy release work (`docs/subsystems/resources/11-cpu-copy-release.md`) by the owner (2026-10-03): a different mechanism from the release /
reload of GPU-uploaded copies.

**The frames were held TWICE — FIXED 2026-10-04** (owner: "image or owned pixmap"): a
`MovieResource::Frame` now either SHOWS a store `ImageResource` (kept alive, never copied) or OWNS its pixels (the
generated frames: debug, noise, parametric, `load(frames)`, the WAD flats); `Frame::pixmap()` answers either, and
`memoryOccupied()` counts only the owned bytes (the shared ones are the images' container's). Citadel census
2690 → 2198 MiB (movies 492 → 0, images held by the store alone 508 → 16 MiB), RSS at rest 5855 → 5515 MiB,
0 VUID. `CubemapMovieResource` got the same `Frame` (a store `CubemapResource`'s six faces, or owned generated
faces; `Frame::faces()`), 2026-10-04: a temporary store movie of two store cubemaps loaded with its frames counted
once (the cubemaps' container +91.5 MiB, the movies' unchanged), 0 VUID; liminal's generated caustics unchanged.
ACCEPTED macOS M2 2026-10-04 (engine e9517d53): citadel census 2659 → 2167 MiB (movies 492 → 0, store-only images
508 → 16), phys_footprint 7387 MB; 0 VUID on citadel and liminal; a fixed-camera A/B shows the torch flames, the
braziers, liminal's water and caustics animating and the static walls at 0 % change.
ACCEPTED Windows (NVIDIA) 2026-10-04: census 2659 → 2167 MiB, private bytes 6943 → 6437 MiB (−506), working set
4617 → 4114 MiB; the brazier flame changes 22-25 % of its pixels between shots, a wall control 0 %.

What stays open here is the residency itself: every frame decoded, for the movie's whole life.

## What remains

1. Find who reads the frames after the animated texture's upload (the frame count and durations are read every frame
   by `BindlessTextureManager::syncTextureSet` / `frameIndexAt`: they must survive as metadata) and whether the GPU
   holds every frame already (an array texture), which would make the CPU frames a pure duplicate.
2. State of the art first: block-compressed frames (BCn / KTX2 array), or streamed decode into a small ring.
3. Owner decision on the approach.

## References

- `src/Graphics/MovieResource.hpp`, `src/Graphics/CubemapMovieResource.hpp`,
  `docs/subsystems/resources/11-cpu-copy-release.md`.
