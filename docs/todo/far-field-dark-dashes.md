---
id: far-field-dark-dashes
title: Dark dashes over the far field of a tiled ground, just under the horizon, on every device
status: open
priority: high
scope: Graphics/TextureResource samplers (anisotropy, LOD bias), Graphics/TextureCompressor + TextureCache (mip chain), Vulkan/Sampler
opened: 2026-10-13
tags: [sampling, mipmaps, cross-platform-bench, defect]
---

# Dark dashes over the far field of a tiled ground, just under the horizon, on every device

## Why

Seen on 2026-09-23 by the macOS (Apple M2, MoltenVK), Windows (RTX 3060) and Linux (RTX 3070 Ti) sessions, in
projet-alpha's `relief` demo, modes 0, 1 and 2 alike: a band of dark horizontal dashes over the
far ground just under the horizon line. On a 2560×1440 frame of the new spawn (mode 1) it starts right under the
horizon (row ≈ 333) and fades over about 80 rows, with a dark fan converging on the vanishing point. A wrong image on
every device: a defect. It is the part of the former AMD item that can be worked without an AMD machine; the AMD-only
sampling difference stays in `amd-far-field-texture-sampling-difference` (parked).

Not re-measured on record since: the same day (engine `530a0d596`, 2026-09-23) the texture cache went to `TextureCache::Version`
3 — every BC7 mip level is now an exact area-weighted box filter (emeraude-base `Processor::downsample()`); version 2
levels were bilinear resamples whose tail lost the mean (`docs/caution-points.md` § "The BC7 mip chain lost the MEAN of
every texture"). A far tiled ground samples exactly those tail levels: the dashes may be gone, or changed.

## What remains

- **Re-measure first** on the current engine (the v3 cache rebuilt: `--clear-renderer-cache` once), `relief` at the
  same spawn and modes, TAA on and off; decode the PNG and measure the band (rows, depth of the dark), never by eye.
- If still there, isolate the cause with a controlled ground (a flat checkerboard at a known frequency) on each device:
  the sampler state (anisotropy `Core/Graphics/Texture/AnisotropyLevels`, 8; mip LOD bias), the mip chain of the
  1024² Pavement006 maps, the TAA (the dashes under motion vs still), the normal / POM inputs (a mode-independent
  symptom points away from the relief technique).
- Then fix in the engine and record the before / after numbers in the topic doc.

## ⚠️ Traps

- Measure a mip chain OFFLINE on the real image before suspecting the shader (the v3 lesson, caution-points).
- A stale texture cache hides a compressor change only if its version was not bumped; v3 is bumped, but clear it once
  to be sure the measured run uses fresh levels.
- Keep the AMD comparison out of this item: without an AMD machine its numbers cannot be re-taken.

## References

- `amd-far-field-texture-sampling-difference` (the AMD-only difference, parked; the 2026-09-22 numbers).
- engine `docs/caution-points.md` § "The BC7 mip chain lost the MEAN of every texture".
- projet-alpha `src/Builtin/Relief.cpp` (the bench).
