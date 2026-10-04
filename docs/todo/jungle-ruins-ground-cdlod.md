---
id: jungle-ruins-ground-cdlod
title: JungleRuins' ground drawn by the engine's CDLOD terrain instead of the raw USD mesh
status: open
priority: unranked
scope: src/Scenes/Loaders/USDLoader, Graphics/Renderable/TerrainResource
opened: 2026-10-04
blocked-by: [geometry-content-dedup]
tags: [terrain, usd, cdlod]
---

# JungleRuins' ground drawn by the engine's CDLOD terrain

## Why

Owner, 2026-10-04: the scene must use the engine's own techniques too, the ground first. The ground is a raw USD mesh
(`elements/Terrain/Terrain_Extended.usd`, 512 MB on disk; very probably the single 792 MB vertex buffer of the
2026-10-04 GPU memory report). Owner choice: the CDLOD terrain over a height clipmap.

## What remains

- Sample the USD terrain mesh into a heightmap (vertical rays over a grid; resolution from the mesh's own density),
  and give the scene an engine terrain with the asset's ground textures instead of the mesh.
- `CDLODTerrainResource` is delivered (2026-09-22, used by `TerrainResource`; only reviews remain in
  `terrain-cdlod-heightmap-clipmap`): check it can take a heightmap built at load time, not only a generated one.
- Then the vegetation (imposters, geometry LOD, wind) — a separate item, in the order the owner sets.
