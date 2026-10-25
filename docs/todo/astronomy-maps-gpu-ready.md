---
id: astronomy-maps-gpu-ready
title: The body maps shipped GPU-ready (BC7 + mips) with their photometry precomputed
status: open
priority: unranked
scope: Graphics (the sky's surface lookup, MoonSurfacePhotometry), tools/texture-bake, projet-alpha.data (Astronomy maps)
opened: 2026-10-25
tags: [sky, planets, textures, bake, design]
---

# The body maps shipped GPU-ready, their photometry precomputed

## Why

Owner decision (2026-10-25, with `sky-body-surfaces-non-blocking`): the data store's astronomy maps
(`Images/Astronomy/<Body>{2K,4K,8K}.png`) are compressed at the customer's on the first launch (15.6 s for an 8K on the
Windows iGPU, a cold cache). The `GPUTextures` store exists for that (graphics § 40, T15: a finished product sends GPU-ready
textures, no texture compute at load).

## What remains

- A lookup of a body's surface in `GPUTextures` first (KTX2, BC7, the mip chain), then `Images` (graphics § 40's resolution
  order, T5) — the sky's surface is not a material component today: name and Purpose to decide (`Color`).
- ⚠️ **The photometry needs pixels**: `MoonSurfacePhotometry::measure()` reads the map's CPU pixels (the near-side mean and
  the 73 × 37 table); a BC7 texture has none — the moon would fall back to a uniform albedo and light. Ship the
  photometry WITH the bake (a small sidecar written by the bake tool: the mean and the table, a few KB), or measure a
  smaller CPU source (the 2K PNG) for an 8K GPU map. The disc's calibration must stay exact (disc ÷ light 1.00).
- The bake: `EmeraudeTextureBake` (or a mode of it) over `Images/Astronomy`, written to `GPUTextures/Astronomy`.

## References

- Graphics doc 42 § "The moons", § 40 (the `GPUTextures` store), § 39 (block formats, the mip filter).
