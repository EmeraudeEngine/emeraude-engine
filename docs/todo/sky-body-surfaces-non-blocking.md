---
id: sky-body-surfaces-non-blocking
title: The sky never waits for a body's surface map — a uniform disc first, the map when it is ready
status: open
priority: unranked
scope: Graphics/Material (SkyAtmosphereResource), Graphics/Renderable (DynamicSkyResource), Scenes/Component (CelestialClock)
opened: 2026-10-25
tags: [sky, moon, planets, loading, design]
---

# The sky never waits for a body's surface map

## Why

A moon's surface map is a DEPENDENCY of the sky material (graphics doc 42 § "The moons"): the whole sky — the
atmosphere, the sun's light from it — waits for the map. Windows-PA measured it (2026-10-25, Intel iGPU, a cold texture
cache): `Astronomy/Moon8K` took 15.6 s of CPU BC7 compression (8192 × 4096, 14 mips) before the sky appeared; with a
warm cache 0.23 s; the 2K map 1.8–1.9 s cold. With every planet and moon to come (item
`dynamic-sky-stars-and-milky-way`), 8K maps for a space game, the first launch would hold the sky for minutes.
Owner decision (2026-10-25): BOTH this item and `astronomy-maps-gpu-ready`.

## What remains

- The sky material created without its bodies' maps: each body drawn first as a UNIFORM disc (its measured albedo and
  colour), its map bound when it is loaded (the descriptor written then; a frame in flight keeps the old binding —
  `DeferredDestructor` / the set ring), the uniform path already exists (`SkyMoonSurfaces` scale 0).
- The light follows: the clock takes the body's photometry when it becomes available (today `followAtmosphere()` reads
  it only when the atmosphere changes — a version counter or a notification), the uniform law until then.
- No visible pop beyond the texture appearing: the uniform disc's mean is the map's (both calibrated to the same mean).

## References

- Graphics doc 42 § "The moons" (failure paths, the photometry), § 40 (the texture cache, `TextureCompressorService`).
