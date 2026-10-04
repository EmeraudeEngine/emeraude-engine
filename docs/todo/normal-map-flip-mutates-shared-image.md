---
id: normal-map-flip-mutates-shared-image
title: Texture2D flips a normal map's green channel IN the shared ImageResource
status: open
priority: unranked
scope: Graphics/TextureResource/Texture2D (createFromPixelData), ImageResource::mutableData()
opened: 2026-10-04
tags: [textures, normal-maps]
---

# Texture2D flips a normal map's green channel IN the shared ImageResource

## Why

`Texture2D::createFromPixelData()` applies `FlipNormalMapY` with `m_localData->mutableData().flipNormalMapY()`: the
pixels of the STORE image are flipped in place, and every texture made from that image shares them. Two textures
with the flag on the same image flip it twice (back to the original); a texture WITHOUT the flag created after one
WITH it reads flipped pixels. Found 2026-10-04 while writing the CPU-copy reload (phase 3 of
`cpu-copies-retained-after-upload`): a released image reloaded from its file comes back UNflipped, so the outcome
also depends on whether a release happened in between.

## What remains

1. Owner decision: flip a copy for the upload (the texture's own staging pixels), or make the flip a property of the
   image (applied once, recorded, refused when two consumers disagree).
2. A runtime check with two materials sharing one normal map, one flipped.

## References

- `src/Graphics/TextureResource/Texture2D.cpp` (`createFromPixelData`), `ImageResource::mutableData()` (its own
  warning says per-instance transformations must go through a copy).
