---
id: instanced-tile-sometimes-escapes-the-culling-after-a-launch
title: About one launch in 25 of a scene of instanced tiles, a tile escapes the culling — its instances are submitted outside the view and cast in every shadow cascade
status: open
priority: high
scope: Scenes (the extent an entity publishes, its render box when a component's mesh finishes loading); found on the testbed's water-world
opened: 2026-11-17
tags: [culling, loading, defect, intermittent]
---

# About one launch in 25 of a scene of instanced tiles, a tile escapes the culling — its instances are submitted outside the view and cast in every shadow cascade

## Why

Found on 2026-11-17 while comparing two binaries on the testbed's `water-world` (about 4460 palms planted on 23 to 25
tile entities, one `MultipleVisuals` per palm model and per tile; Release, RTX 3070 Ti, window-less, the validation
layer loaded, the statistics read 12 s after the start — `Core.SceneManagerService.getRenderStatistics()`):

| | Normal launch | The other state |
|---|---|---|
| view lists | 104 to 108 batches, 2534 to 2596 instances (the palms are planted at random: the count moves a little) | 110 to 114 batches, 3596 to 3640 instances |
| shadow lists (4 cascades) | the same batches and instances as the view, plus 4 | 134 to 142 batches, 5676 to 5768 instances |
| frames rendered in the same run | 3396 | 3115 |

- **Intermittent and decided at the start**: the state holds for the whole run (two reads 4 s apart give the same
  totals). 3 launches out of 74: 1 of 30 with the binary of engine f10bc200, 2 of 44 with the working tree of
  2026-11-17 — NOT brought by the changes in between. The three occurred in launches that alternated two binaries; 14
  launches of one binary in a row gave none (too few to say the alternation matters).
- **The picture is the same** (one capture of each state, looked at): the same framing, the same palms. The 1050
  extra instances are outside the view, and the extra shadow batches are tiles cast in cascades they do not reach.
  The cost is real: 8 % fewer frames in the same time.
- The camera is the same in both states (`Camera.getState`: view distance 25000 m), the logs are the same line for
  line, the planted count is the same (4441 to 4467).

**Candidate mechanism, NOT verified**: one tile's 6 batches (6 palm models) explain the numbers — +6 batches in the
view, the same 6 in every cascade. An entity is culled on the extent it publishes
(`AbstractEntity::publishRenderExtent()`): its render box when valid, else its collision box, else the POSITION of its
coordinates. A tile sits at the centre of its cell at sea level: tested on its position, it is "seen" whenever that
point is in the frustum, wherever its palms are, and `missesCascadeReceivers()` only culls an extent of the
`RenderBox` kind. So a tile whose render box stayed INVALID fits — a render box is merged from the components' boxes
by `updateEntityProperties()` when the component is linked, and a `MultipleVisuals` built on a mesh still loading has
no box until the load ends and `ComponentContentModified` recomputes it (scenes doc 12 § *A resource's load reaches a
scene object on the logic thread*). A load that ends in a window where that notice is lost would leave the tile
without its box for the whole run.

## What remains

1. **Verify the mechanism first**: trace the extent kind (and box) each `PalmTile*` entity publishes a few seconds
   after the start, launch until the state shows (about 1 in 25: the script of 2026-11-17 alternates two binaries and
   reads the totals), compare with a normal launch. If a tile publishes `Point`, find why its render box is invalid:
   the order between the end of the mesh's load and the component's link / its first `processLogics()`.
2. Fix it where the notice is lost (never by recomputing every cycle), prove it by the same launches — none in a
   number of launches that would have shown it (at 4 %, 75 clean launches leave 5 % of chance), or better a
   deterministic reproduction (a delay injected in the mesh's load).
3. Decide with the owner what an entity that DRAWS but has no measurable box should do meanwhile: today it is culled
   on its position and cast in every cascade — silently.

## ⚠️ Traps

- The scene is random (the palms' positions): never compare two launches' totals to the unit; the two states are
  1000 instances apart.
- A statistics A/B on this scene needs many launches: this state made a clean change look like a regression
  (2026-11-17, 2 occurrences with the new binary before the old one showed its first).

## References

- `src/Scenes/AbstractEntity.cpp` (`publishRenderExtent()`, `updateEntityProperties()`), `src/Scenes/Scene.rendering.cpp`
  (`isSeenBy()`, `missesCascadeReceivers`).
- The testbed's `src/Builtin/WaterWorld.cpp` (the palm tiles).
