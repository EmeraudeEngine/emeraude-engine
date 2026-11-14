---
id: render-target-download-refuses-hdr-color-formats
title: No HDR render target can be read back — the dump command fails on every environment probe
status: open
priority: high
scope: Vulkan/TransferManager (downloadImage), Scenes/Manager.console (dumpRenderTarget), Graphics/RenderTarget (capture)
opened: 2026-11-14
tags: [defect, render-target, console, hdr, owner-decision]
---

# No HDR render target can be read back — the dump command fails on every environment probe

## Why

`TransferManager::downloadImage()` reads the 8-bit colour formats (RGB(A) / BGR(A), UNORM or sRGB) and the depth /
stencil formats, into a `Pixmap< uint8_t >`. Any other format is refused (owner ruling 2026-10-01: sized as RGBA8, a
16-bit target overflowed its staging buffer). The environment probes are `R16G16B16A16_SFLOAT`
(`Scene::createRenderToCubemap()`, 16 colour bits by default, since the probes stopped clamping photometric scenes), so:

- `Core.SceneManagerService.dumpRenderTarget(<probe>)` answers "Unable to download the image", with
  `The image '…' has the format 97, which cannot be downloaded` in the log — measured 2026-11-14 on
  `offscreen-rendering` (`OffscreenRenderingCubemap`), every layer;
- `docs/ai-runtime-control.md` presents that command as the ground-truth tool for "what did the probe actually bake?":
  it has not worked on a probe since the probes became HDR;
- `RenderTarget::Texture::capture()` fails the same way on such a target (no caller today).

The layer argument of the command (2026-11-14) was proven on a probe created in 8 bits for the measurement only.

## What remains

- **Owner decision 2026-11-14 — both forms:** an 8-bit PNG through a stated mapping (an exposure argument, then the
  sRGB transfer; a clamp alone gives a white image on a photometric scene) AND a float file that keeps the values
  (the base reads Radiance `.hdr`; check what it WRITES before choosing the container). The refusal of an unknown
  format stays.
- `downloadImage()` learns `R16G16B16A16_SFLOAT` (8 bytes per pixel, the staging buffer sized from the real format);
  the float pixels reach the caller as floats, and the command writes the two files and documents its mapping.
- Verify: the six faces of `OffscreenRenderingCubemap` dumped without any local change, 0 `VUID-`.

## References

- `src/Vulkan/TransferManager.cpp` — `downloadImage()` (the format switch, its refusal).
- `src/Scenes/Scene.rendering.cpp` — `createRenderToCubemap()`; `src/Vulkan/Instance.cpp` — `findColorFormat()`.
- `docs/render-targets.md` § *Reading a target back*.
