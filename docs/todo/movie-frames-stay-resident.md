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

**The frames are held TWICE** (found 2026-10-04): `MovieResource::load()` / `loadManual()` COPY each frame's pixmap
out of a store `ImageResource` (`m_frames.emplace_back(imageResource->data(), …)`), and the store keeps that image.
Citadel: 508 MiB of the 775 MiB of images are held by the store alone (`memoryCensus()` `unusedBytes`) against
492 MiB of movie frames — the same pixels. Holding the `std::shared_ptr< ImageResource >` (or moving the pixmap out
of an image only the movie uses) removes the copy without touching the streaming question.

## What remains

1. Find who reads the frames after the animated texture's upload (the frame count and durations are read every frame
   by `BindlessTextureManager::syncTextureSet` / `frameIndexAt`: they must survive as metadata) and whether the GPU
   holds every frame already (an array texture), which would make the CPU frames a pure duplicate.
2. State of the art first: block-compressed frames (BCn / KTX2 array), or streamed decode into a small ring.
3. Owner decision on the approach.

## References

- `src/Graphics/MovieResource.hpp`, `src/Graphics/CubemapMovieResource.hpp`,
  `docs/todo/cpu-copies-retained-after-upload.md` § Phase 0.
