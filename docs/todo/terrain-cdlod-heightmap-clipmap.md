---
id: terrain-cdlod-heightmap-clipmap
title: CDLOD terrain over a height clipmap — replace AdaptiveVertexGridResource
status: in-progress
priority: unranked
scope: Graphics/Geometry, Graphics/Renderable/TerrainResource, Saphir
opened: 2026-09-22
tags: [terrain, cdlod, clipmap, saphir, ray-tracing]
---

# CDLOD terrain over a height clipmap

## Why

`AdaptiveVertexGridResource` stores one 56-byte vertex per grid point (940 MB for the 4 km window
of the `terrain` demo), re-uploads it whole at every slide, stitches LODs with fans, needs a far
mesh with a skirt around the window, and lights every LOD with the ONE 1 m normal of each vertex —
the dark streaks on distant ridges. Owner, 2026-09-22: "Go option B".

## Owner decisions (2026-09-22)

1. **Heights on the GPU = a clipmap of R16 levels**, 2048² texels each, level `l` has a texel of
   `cell · 2^l`, camera-centred, updated TOROIDALLY by strips. It replaces both the streamed
   window and the far mesh. (The 16 385² grid of the demo is one texel over the device's
   `maxImageDimension2D` of 16 384: a single full-resolution texture is impossible anyway.)
2. **Normals per PIXEL from a BAKED texture**: a compute pass bakes one normal map per clip level
   (from that level's heights), read by the fragment stage. Needs a Saphir "surface frame" entry
   point in the fragment stage.
3. **Per-node constants through PUSH CONSTANTS**: one push + one draw per selected node.
4. **`AdaptiveVertexGridResource` is REPLACED then DELETED** once parity is measured (its only user
   is `TerrainResource`).

## Design (as implemented — keep in sync)

- One shared patch: `(G+1)²` flat vertices (position only), triangle list. A node of LOD `k`
  covers `leafSize · 2^k` metres; its vertices are displaced in the vertex stage by sampling
  clip level `k` (clamped to the last level), geomorphed toward the LOD `k+1` lattice in the last
  part of its range (Strugar's `morphK`), and the height blends from level `k` to level `k+1` with
  the SAME `morphK` so a fully morphed vertex reads exactly what the coarser neighbour reads — no
  stitching, no crack.
- Clip levels hold MEAN-filtered heights (1-2-1 tent pyramid computed on the CPU at load), so a
  coarse LOD is a low-pass of the surface, not an aliased point sample; level 0 is the exact grid.
- The fragment picks the normal level from the pixel footprint (`fwidth` of the world XZ), blends
  the two nearest levels, and never picks a level that does not cover the pixel.
- The LOD is chosen from the MAIN camera in every pass (shadows included), culling by each pass's
  own frustum — the rule already applied by the adaptive grid.
- Ray tracing gets a DEDICATED proxy (vertex + index buffers) regenerated from the CPU grid under
  `TerrainBLASMaxTriangles`, through a new `rtVertexBufferObject()` symmetric to
  `rtIndexBufferObject()`.
- Physics keeps answering from the CPU `Grid< float >`.

## What remains

Delivered 2026-09-22 (base `Grid::halvedTent()`, engine `CDLODTerrainResource` + Saphir heightfield
surface, `AdaptiveVertexGridResource` deleted; knowledge in `src/Graphics/AGENTS.md` § "Adaptive
geometries", `src/Saphir/AGENTS.md` § "Heightfield surface", `docs/caution-points.md` § CDLOD terrain).
Measured at the pushed adaptive grid's pose: ScenePass 3.27 ms (SS lane) / 2.32 ms (RT lane) against
2-3.4 ms, process VRAM 1821 MiB against 2791 MiB, 0 VUID. Left open:

- [ ] Owner review in motion: morph (no popping expected), seams, the lit look of distant ridges.
- [ ] Measure a clip level re-centre while flying (strip copy + bake, submitted ahead of the frame): no
  number yet, only the first full upload (40 MiB, 113 ms at boot).
- [ ] Small isolated bright/dark cones are visible on the relief seen from 1.2 km: decide whether they
  are the diamond-square's own spikes (they are on the CPU grid too?) or a clipmap/normal artefact —
  measure on the source grid before touching the shader.
- [ ] Decide the R16 quantum (6 cm over 4000 m) once lit gentle slopes have been looked at closely.

## ⚠️ Traps

- Push constants: the existing blocks go up to 76 B; the node vec4 lands at offset 80 (std430
  alignment) — still under the 128 B guarantee. Never add a second per-node vec4 there.
- Frames in flight read the clip textures while the next frame updates them: the update is
  recorded on the GRAPHICS queue before the frame's first pass, behind a barrier, never through
  the transfer queue's layout transitions.
- A teleport (camera moving more than a level's slack in one frame) needs a full level rewrite.

## References

- F. Strugar, "Continuous Distance-Dependent Level of Detail for Rendering Heightmaps", JGT 2009 —
  https://github.com/fstrugar/CDLOD (node selection, morph formula).
- F. Losasso, H. Hoppe, "Geometry Clipmaps", SIGGRAPH 2004 (toroidal level updates, transition
  blend between levels).
