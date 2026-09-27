---
id: mesh-shading-surface-on-heightfield
title: Real displaced micro-geometry on a heightfield terrain (TerrainResource) through task + mesh shaders
status: open
priority: unranked
scope: Graphics/Geometry (CDLODTerrainResource, MeshShadingSurface), Saphir/Generator (HeightfieldSurfaceHelper, MeshShadingSurfaceHelper)
opened: 2026-09-27
blocked-by: [mesh-shader-displaced-surface]
tags: [mesh-shaders, relief, terrain, cdlod, heightfield]
---

# Real displaced micro-geometry on a heightfield terrain (TerrainResource) through task + mesh shaders

## Why

Owner (2026-09-27): projet-alpha's `water-world` island moved to a `TerrainResource` with the store material
`Grounds/Sand001`, which asks for "relief through mesh shaders, POM as the fallback". The engine cannot do it:

- the mesh-shading surface (`Geometry::MeshShadingSurface`, `DisplacedGridResource`,
  `Saphir::Generator::MeshShadingSurfaceHelper`) is a FLAT rectangle of square tiles, displaced only by the
  material's height map;
- `TerrainResource` is a heightfield drawn by CDLOD over a height clipmap (`Geometry::HeightfieldSurface`,
  `HeightfieldSurfaceHelper`): its program is a VERTEX stage that reads the clipmap and geomorphs between
  levels. A material's `ParallaxHandover` is ignored there, so the sand is plain POM on every device.

Owner decision (2026-09-27, AskUserQuestion): deliver `water-world` with POM first, then design this item together.

## What remains

Everything — this is a design item. Questions to put to the owner before any code:

1. **Where the mesh path lives**: the CDLOD nodes closer than the handover band re-emitted by a task + mesh
   program that samples the clipmap height AND adds the material's displacement, the far nodes staying on the
   vertex path (two programs, one terrain) — or a dedicated near-camera mesh-shading patch laid over the
   heightfield (a second renderable: z-fight and seams to solve).
2. **Crack handling** between a re-tessellated node and its CDLOD neighbours: the flat surface chose SKIRTS
   (owner, 2026-09-23); the heightfield already geomorphs — which one wins at the band?
3. **The frame**: the heightfield rebuilds T/B/N per pixel from its normal clipmap
   (`FragmentShader`, `HeightfieldFrameOverrides`); the mesh stage must hand over the same position varying so
   the fragment code stays shared.
4. **Shadows**: the flat surface's shadow program is subdivided for the MAIN camera
   (`ShadowCasting::generateMeshShadingStages()`); the same has to hold for the heightfield.
5. **Cost**: the flat surface still costs +3.2 ms colour / +0.5 ms shadow on an RTX 3070 Ti after culling
   (`mesh-shader-displaced-surface`); a terrain covers far more ground — measure before choosing.

## ⚠️ Traps

- POM on the CDLOD path has never been judged before 2026-09-27 (`water-world` is the first heightfield with a
  height map): check it against a flat `DisplacedGridResource` with the same material before blaming the mesh path.
- The height map must be a HEIGHT (`src/Graphics/AGENTS.md` § Parallax Occlusion Mapping). `Sand001`'s normal
  map is its height map's exact derivative (Frankot-Chellappa integral, correlation 0.99998, Khronos
  convention), and describes a relief of 7 texels of 2048: `Scale` 0.0034 UV.
- MoltenVK has no `VK_EXT_mesh_shader`: the fallback must stay the vertex CDLOD path with POM.

## References

- engine `docs/todo/mesh-shader-displaced-surface.md` (the flat surface, its decisions and measurements);
- engine `docs/todo/terrain-cdlod-heightmap-clipmap.md` (the CDLOD terrain);
- F. Strugar, "Continuous Distance-Dependent Level of Detail for Rendering Heightmaps" (JGT, 2009).