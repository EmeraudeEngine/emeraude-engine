---
id: image-download-depth-formats-always-fail
title: downloadImage() can never fill a pixmap from a depth image — four bytes per pixel into a one-channel 8-bit pixmap
status: open
priority: high
scope: Vulkan/TransferManager (downloadImage, the 8-bit overload), Graphics/RenderTarget (capture)
opened: 2026-11-14
tags: [defect, render-target, depth, dead-path, owner-decision]
---

# downloadImage() can never fill a pixmap from a depth image

## Why

The 8-bit overload of `TransferManager::downloadImage()` accepts `D32_SFLOAT`, `D32_SFLOAT_S8_UINT` and
`D24_UNORM_S8_UINT`: it sizes the staging buffer at FOUR bytes per pixel (right: the depth aspect of those formats is
copied as 32 bits) and declares the pixmap `ChannelMode::Grayscale`. It then calls
`pixmap.initialize(width, height, Grayscale, {pointer, width × height × 4})` on a `Pixmap< uint8_t >`, whose
`initialize()` requires the data to hold exactly `elementCount()` values — `width × height` for one 8-bit channel. The
sizes never match: `initialize()` returns false (a debug log in a Debug build, nothing in Release) and the download
fails AFTER the GPU copy. Only `S8_UINT` (one byte per pixel) can succeed.

Read on 2026-11-14 while adding the floating-point overload; **not run**: the only callers that pass a depth aspect
are `ShadowMap::capture()`, `Texture::capture()` and `SceneRenderTarget::capture()`, and nothing calls a `capture()` in
the cascade (a `Texture`'s depth image has no `TRANSFER_SRC` anyway). A shadow map has therefore never been read back
through this path.

## What remains

- **Owner decision — what a depth read-back returns**: the depth as floats (a `Pixmap< float >` of one channel: the
  floating-point overload would learn the depth formats; `D24_UNORM` needs its own decoding, 24 bits in the low bytes),
  or an 8-bit greyscale through a stated mapping (linearised with the near / far planes, or raw).
- A caller to prove it: `dumpRenderTarget` walks the render-to-textures only, a shadow map has no dump command.
- Until then the 8-bit overload should not pretend: refuse the depth formats with a message, as it refuses an unknown
  format, instead of copying and failing in `initialize()`.

## References

- `src/Vulkan/TransferManager.cpp` — `downloadImage()` (the format switch of the 8-bit overload).
- emeraude-base `src/PixelFactory/Pixmap.hpp` — `initialize(width, height, channelMode, data)`.
- `docs/render-targets.md` § *Reading a target back*.
