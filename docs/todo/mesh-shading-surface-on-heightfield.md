---
id: mesh-shading-surface-on-heightfield
title: Real displaced micro-geometry on a heightfield terrain (TerrainResource) through task + mesh shaders
status: in-progress
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

## Owner decisions (2026-09-28, AskUserQuestion)

1. **A camera-following mesh-shading WINDOW plus a HOLE in the CDLOD** (chosen over "near nodes switch pipeline
   inside the CDLOD draw" and over "a DisplacedGridResource with a base height texture, small terrains only"): the
   window is made of whole level-0 CDLOD nodes around the camera; the CDLOD skips exactly those nodes. The window
   reads the terrain's height clipmap as its base height and adds the material's displacement on top. Its border lies
   beyond the geometry-to-parallax handover, where the relief is flat, so it meets the CDLOD on the lattice heights
   (skirts against the residual cracks). Without `VK_EXT_mesh_shader`: no window, CDLOD + POM everywhere. It must
   hold for `terrain`'s 16 km too, not only `water-world`.
2. **Deepen `Grounds/Sand001`** so the result is visible on `water-world`: its 3.4 mm relief is 2-3 px at 2 m, which
   POM already shows; raise `Height.Scale` AND `Normal.Scale` together (the lighting must keep describing the same
   relief), measure, the owner judges the image.

3. **The window is a COMPANION renderable** (chosen over generalising `Renderable::Abstract` to one geometry per
   layer): `TerrainResource` owns it (mesh-shading geometry + the same material) and exposes it through
   `GroundLevelInterface` (optional, null without `VK_EXT_mesh_shader`); the scene registers it as a fourth scene
   visual beside the ground. The `Renderable` contract, `RenderableInstance` and the program cache are untouched.

⚠️ Data trap found while sizing: `Grounds/Mud001` (the `terrain` ground) has `Height.Scale = 1.0` — a relief one whole
texture repeat deep (8 m at `terrain`'s tiling). Harmless while `POMIterations = 0` skips its POM; a mesh-shading
window would displace it by metres. Fix the data before `terrain` gets the window.

## What remains — the plan (2026-09-28)

The window is a PURE FUNCTION of the pass's camera position: the 2 × 2 block of level-0 node QUARTERS nearest to the
camera (32 m quarters at 1 m cells, so a 64 m window whose centre is never more than 16 m from the camera — the 5 m
handover disc always fits). The CDLOD hole and the window are both computed from that function with the camera of
the pass (`lodViewPosition`, the MAIN camera in a shadow pass), so no state is shared between the two geometries and
no thread has to publish anything.

1. **The hole** (`CDLODTerrainResource`): when the detail window is enabled, `selectNode()` pushes a level-0 node
   quarter by quarter (the patch index buffer is already sorted by quarter) and skips the quarters inside the window.
   Testable alone: a 64 m hole that follows the camera.
2. **The window geometry** (new `Graphics::Geometry` class): `EnableMeshShadingSurface` over the window rectangle,
   whose origin moves with the camera — `drawMeshShadingSurface()` needs the rectangle per draw (a per-camera
   accessor on `Geometry::Interface`, the static `meshShadingSurface()` for a flat grid).
3. **Saphir** (`MeshShadingSurfaceHelper`): a base height from the terrain's clipmap (`hfHeight()`, level 0, the
   heightfield set bound as the program's `PerModel` set with TASK | MESH stages), the material's displacement on top,
   the heightfield per-pixel frame in the fragment stage; tiles split along the CDLOD diagonal; the flat tiles near
   the window border reproduce the level-0 GEOMORPH so they meet a morphing CDLOD neighbour exactly.
4. **The companion renderable**: `TerrainResource` creates it only when the device has `VK_EXT_mesh_shader` (the
   mesh-shading geometry's own vertex fallback would draw a flat grid without the terrain's heights); exposed by
   `GroundLevelInterface`, registered by the scene as a fourth scene visual.
5. **Shadows**: the mesh shadow program with the heightfield set; the CSM passes (terrain's default) — the mesh task
   stage does not cull against a cascade today.
6. **`water-world` + Sand001 deepened**: measure the frame and the GPU cost against the POM, the owner judges.

## Progress (2026-09-28)

Steps 1-4 are implemented and run on `water-world` (RTX 3070 Ti, validation ON, 0 VUID, no shader error): the hole
follows the camera, the companion window fills it, the aerial view is identical to the pure CDLOD on the terrain
(shadows included), the spawn frame differs from CDLOD + POM on 1.18 % of the pixels. Skirts are hung only toward a
COARSER neighbour: on every edge they drew bright one-pixel lines along the convex folds (1.40 % → 1.18 %).
`Sand001` deepened ×5 (`Height.Scale` 0.017, `Normal.Scale` 5): the ripples read in both modes.

OPEN: at a close pose with the deepened sand, the window shows darker, straight-edged blocks inside the ray-traced
cast shadow, absent in POM mode and gone with shadow mapping disabled. A shadow bias of 0.05 (10×) lightens them without
removing them: shadow acne of the 17 mm relief on 14 cm texels is NOT established — a real cast shadow of the
neighbouring displaced geometry, quantised by the texels, is as likely. Captures `1790552389` (mesh), `1790552410` (POM),
`1790552495` (mesh, no shadow map).

## ⚠️ Traps

- ⚠️⚠️ The mesh tiles split their quads along (x, z + 1)–(x + 1, z) (`MeshShadingSurfaceHelper`, the Grid
  convention), the CDLOD along (x, z)–(x + 1, z + 1). Over a base heightfield they MUST take the CDLOD's diagonal:
  the ray-tracing proxy is split that way, and a traced surface that parts from the drawn one blackens RTAO / RTGI /
  RTContactShadows (docs/caution-points.md § CDLOD terrain, fixed 2026-09-27 for the proxy itself).
- The CDLOD's level-0 geomorph may be active at the window border (its morph starts at 0.66 of the level-0 range):
  an unmorphed window edge against a morphing neighbour cracks by the height difference between the levels —
  metres on a rough relief — far more than a skirt should hide.

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