---
id: terrain-pack-vertex-and-height-data
title: Pack the terrain's vertex data and its source heights — 56 bytes per heightfield vertex is the limit on the window
status: open
priority: unranked
scope: Graphics/Geometry
opened: 2026-09-22
tags: [terrain, memory, vertex-format, saphir]
---

# Pack the terrain's vertex data and its source heights

## Why

An `AdaptiveVertexGridResource` vertex is **14 floats = 56 bytes**: position (3), tangent (3),
binormal (3), normal (3), UV (2). On the `terrain` demo the 4096 × 4096 window is **16 785 409
vertices = 940 MB of VBO**, regenerated and re-uploaded whole at every slide (896 MiB in 36 ms,
measured), and that size is what caps the window at 4 km: 8 km would be 3.6 GB. The source heights
are `float` too: 16 385² × 4 B = **1.07 GB** of CPU for the 16 km grid.

For a HEIGHTFIELD almost none of this is information:
- X and Z are the grid index times the cell size plus the window offset — derivable in the vertex
  shader from `gl_VertexIndex` and two uniforms; only **Y** is data.
- The tangent frame of a heightfield is a function of the height's gradient: T follows +X, B follows
  +Z, N = normalize(cross). Either derive it in the shader from the neighbours' heights (a height
  texture makes that a texel fetch) or store one octahedral-encoded normal (2 × 16 bits) and rebuild
  T/B from it and the grid axes.
- UVs are X and Z scaled.

So a vertex could be **2 to 8 bytes** (a 16-bit height alone at 6 cm of quantisation over 4000 m,
or height + packed normal), i.e. the same window at **34-134 MB** instead of 940, or the WHOLE 16 km
grid resident at 16 bits in 537 MB — which is what makes the CDLOD rework (heights in a texture,
one shared patch mesh) the natural end of this road.

Owner (2026-09-22): "On va voir comment réduire la taille du VBO en packant les données, pareil pour
les données sources."

## What remains

- [ ] Decide the vertex format: (a) `R16_UNORM` height only + derived frame, (b) height + octahedral
  normal, (c) heights in a texture read by the vertex stage (= CDLOD's layout). Each needs a Saphir
  vertex-input path: the generator today emits the fixed float layout of `getElementCountFromFlags()`
  and the material's TBN expects per-vertex tangent space.
- [ ] Decide the source storage: `Grid< float >` → a 16-bit grid (`Grid< uint16_t >` + scale/offset,
  or a quantised view over the same class) — `getLevelAt()` (physics, spawn) interpolates it either
  way.
- [ ] Measure the quantisation: 16 bits over the relief's range is 6 cm on `terrain` (4000 m) and
  0.5 mm on `forest` (34 m); a fixed-point height needs the RANGE known at load (it is: the bounding
  box).

## ⚠️ Traps

- The RT proxy (`generateTriangleListIndicesForRT()`) reads positions from the VBO through the grid
  index; a packed vertex changes what the hit shader can read for the terrain (position from index,
  normal from the packed attribute).
- The `Grid` heights are also what `TerrainResource::getLevelAt()` answers the physics with: a
  quantised source moves every collision by up to half a step.

## References

- `src/Graphics/Geometry/AdaptiveVertexGridResource.cpp` — `writeVertex()`, the 14-float layout.
- `src/Graphics/Geometry/Types.hpp` — `getElementCountFromFlags()`.
- `src/Saphir/VertexShader.hpp` — the vertex input declarations the format must go through.
