---
id: lit-lists-drawn-per-batch-without-mdi
title: The LIT render lists are drawn batch by batch, outside multi-draw-indirect — lighting a forest costs ~45 ms
status: open
priority: high
scope: Scenes/Scene.rendering, Graphics/MDI
opened: 2026-09-25
tags: [performance, mdi, vegetation, measured]
---

# The LIT render lists are drawn batch by batch, outside multi-draw-indirect

## Why

⚠️ **Corrected 2026-09-25 (code reading of the hang diagnosis): the unlit forest was NOT drawn through MDI.**
`Core/Graphics/MDI/Enabled` defaults to false, and the MDI `BatchBuilder` cannot draw an instanced batch anyway
(it hard-codes `instanceCount = 1`, always takes `geometry(0)`, and reads its model matrix from a per-draw SSBO:
see `mdi-draws-instanced-batches-wrong`). Before the trees were lit, the forest went through the per-batch "Phase
1A" loop of the `Opaque` list: ONE `render(SimplePass)` per batch, state-sorted so consecutive batches share a
pipeline. The `OpaqueLighted` list is drawn by `renderLightedSelection()` batch by batch: one AmbientPass plus one
DirectionalLightPassCSM (plus one per extra light) for EACH batch. The two pipelines ALTERNATE, and every pipeline
change calls `invalidateDescriptorSets()`, so each draw re-binds the view, material and bindless sets (plus the
light set). The lit delta is therefore 1 → 2 draws and ~2 pipeline switches + ~4-5 descriptor binds per batch —
not "MDI versus per batch".

The sun pass also runs a `(2·PCFSamples + 1)²` tap loop per fragment (81 taps at 4, 289 at 8 — the Windows peer's
value), doubled in the cascade blend band, with no early exit beyond the cascades: the imposters out to 6 km pay it
(`csm-light-pass-pcf-beyond-cascades`).

The forests of `terrain` became lit on 2026-09-25 (they had been unlit by omission). The frame, GPU, validation ON,
frozen noon sun, same poses, before → after — ⚠️ measured WITH the octree gather duplicating entities (fixed the
same day, engine `docs/caution-points.md` § *the octree-culled render lists drew an entity once per sector copy*),
so the absolute numbers are inflated; re-measure:

| pose | unlit trees | lit trees |
|---|---|---|
| startup pose (forest 1-2 km, imposters), traced lane | 23.1 ms (scene pass 4.6) | 74.9 ms (scene pass 51.3) |
| close to trees, screen-space lane | 37.3 ms (scene pass 20.5) | 73.6 ms (scene pass 56.0) |
| close to trees, traced lane | 63.8 ms | 111.8 ms |

Owner, 2026-09-25: **"the visual result comes before the performance"**. The lit forest stays; this item is
where the cost is to be won back, never by unlighting anything.

## What remains

State on 2026-10-11 (board audit): `renderLightedSelection()` is still batch-major (`Scene.rendering.cpp` ~2241-2244).
But two things changed around this item:
- The unshadowed point and spot lights moved to the deferred resolve (8c461d12, item `mrt-single-pass-deferred`).
- The owner weighed the former step 2 on 2026-10-04 (record the lit selection PASS-MAJOR, ambient first) and dropped
  it: it only touches the fragment share (`mrt-single-pass-deferred` § What remains).

What is left of the forest's cost is the sun's CSM pass. That is `mrt-single-pass-deferred`'s "the SUN in the
resolve" step, plus `csm-light-pass-pcf-beyond-cascades`. Left here:

1. Re-measure the table with the deduplicated gathers and the deferred resolve. If the lit lists are no longer the
   cost, close this item into those two.
2. Profile what remains: draw count against fragment cost (the PCF loop), imposter batches against mesh batches.
3. Only then consider indirect draws for the lit lists. That first needs MDI to support instanced batches and LODs
   at all (`mdi-draws-instanced-batches-wrong`).

## References

- Diagnosis of the unlit forests: projet-alpha `docs/caution-points.md` § *a visual's lighting state is now a
  REQUIRED argument*.
