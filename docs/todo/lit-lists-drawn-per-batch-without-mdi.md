---
id: lit-lists-drawn-per-batch-without-mdi
title: The LIT render lists are drawn batch by batch, outside multi-draw-indirect — lighting a forest costs ~45 ms
status: open
priority: unranked
scope: Scenes/Scene.rendering, Graphics/MDI
opened: 2026-09-25
tags: [performance, mdi, vegetation, measured]
---

# The LIT render lists are drawn batch by batch, outside multi-draw-indirect

## Why

The unlit `Opaque` list goes through the MDI batch builder (`Scene.rendering.cpp`, ~l. 760-766). The
`OpaqueLighted` list is drawn by `renderLightedSelection()` batch by batch: one AmbientPass plus one
DirectionalLightPassCSM (plus one per extra light) for EACH batch.

The forests of `terrain` became lit on 2026-09-25 (they had been unlit by omission). That is 37 335 mesh +
37 335 imposter instanced visuals. The frame, GPU, validation ON, frozen noon sun, same poses, before → after:

| pose | unlit trees | lit trees |
|---|---|---|
| startup pose (forest 1-2 km, imposters), traced lane | 23.1 ms (scene pass 4.6) | 74.9 ms (scene pass 51.3) |
| close to trees, screen-space lane | 37.3 ms (scene pass 20.5) | 73.6 ms (scene pass 56.0) |
| close to trees, traced lane | 63.8 ms | 111.8 ms |

Owner, 2026-09-25: **"the visual result comes before the performance"**. The lit forest stays; this item is
where the cost is to be won back, never by unlighting anything.

## What remains

1. Profile where the 45 ms go: draw-call count, pipeline and descriptor rebinds per batch, the two passes per
   batch, the imposter batches versus the mesh batches.
2. Extend MDI to the lit lists: the ambient pass and each light pass as indirect draws over the same batch
   set, grouped by pipeline.
3. Consider a single forward pass (ambient + sun) for instanced vegetation, if the pass split itself is the
   cost.
4. Re-measure the table above, and `forest`.

## References

- Diagnosis of the unlit forests: projet-alpha `docs/caution-points.md` § *a visual's lighting state is now a
  REQUIRED argument*.
