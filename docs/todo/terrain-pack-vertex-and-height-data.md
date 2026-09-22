---
id: terrain-pack-vertex-and-height-data
title: Pack the terrain's CPU source heights — a 16 km grid is 1 GB of floats
status: open
priority: unranked
scope: Graphics/Renderable/TerrainResource, emeraude-base VertexFactory
opened: 2026-09-22
tags: [terrain, memory]
---

# Pack the terrain's CPU source heights

## Why

The VERTEX half of this item is gone: since 2026-09-22 the terrain is a CDLOD patch displaced by a
16-bit height clipmap (`Geometry::CDLODTerrainResource`), no terrain vertex is stored and the GPU side
went from a 940 MB window VBO to ~120 MB of clipmaps. What is left is the CPU source: the whole grid is a
`Grid< float >` — 16 385² × 4 B = **1.07 GB** on `terrain` — shared by the physics (`getLevelAt()`),
the clip level 0 uploads and the ray-tracing proxy. The coarse levels of the pyramid are already stored
at 16 bits (170 MiB).

Owner (2026-09-22): "On va voir comment réduire la taille du VBO en packant les données, pareil pour
les données sources."

## What remains

- [ ] Decide the source storage: `Grid< float >` → 16 bits (`Grid< uint16_t >` + scale/offset, or a
  quantised view over the same class) — the clip level 0 already quantises to the same 16 bits.
- [ ] Measure what the physics loses: 16 bits over the relief's range is 6 cm on `terrain` (4000 m) and
  0.5 mm on `forest` (34 m); a collision moves by up to half a step.

## ⚠️ Traps

- The ray-tracing proxy and its normals are built from this grid on a worker: a quantised source must
  still give smooth normals (derive them from the quantised heights and look at a gentle slope).

## References

- `src/Graphics/Geometry/CDLODTerrainResource.cpp` — `levelHeight()` (the level-0 quantisation), `generateRayTracingProxy()`.
- `src/Graphics/Renderable/TerrainResource.hpp` — `getLevelAt()`, `m_localData`.
