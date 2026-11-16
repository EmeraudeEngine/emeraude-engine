---
id: scene-publication-republishes-every-entity-at-every-cycle
title: The scene publishes every entity again at every logic cycle — 4.7 ms per cycle on a terrain of 13072 static entities
status: open
priority: unranked
scope: Scenes (Scene::publishStateForRendering(), AbstractEntity::publishStateForRendering(), StaticEntity / Node::onPublishStateForRendering(), the components' publication)
opened: 2026-11-16
tags: [performance, measured]
---

# The scene publishes every entity again at every logic cycle — 4.7 ms per cycle on a terrain of 13072 static entities

## Why

`Scene::publishStateForRendering()` walks every static entity and every node at the end of each logic cycle; each one
copies its coordinates into the render state slot, publishes its render extent and dispatches the publication to its
components (a copy of the component list under the entity's lock) — whether anything changed or not. The code carries
the note itself: "TODO: Check to copy only relevant data to speed up the transfer".

Measured on 2026-11-16 (Release, i9-14900K, a temporary timer around the two entity loops, the mean of 600 cycles,
four launches): the testbed's `terrain`, 13072 static entities that never move, **4.5 to 5.1 ms per logic cycle**
(about 360 ns per entity) — against a logic cycle of 16.7 ms at 60 Hz. `city`, 721 static entities and its nodes: 62 to
79 µs. With the render extent published since that day: 5.1 to 5.5 ms and 70 to 102 µs (an unchanged entity copies its
previous extent; computing it for every entity cost 5.9 to 6.6 ms).

## What remains

- Not ranked by the owner (a performance gain, not a defect).
- Decide the design (owner): an entity that changed nothing since its last publication still has to be right in the
  slot being written — the triple buffer rotates, so "unchanged" means "the same as the slot written LAST", and a slot
  not written for two cycles holds an older state. Candidates: a dirty count per entity (the number of slots still to
  refresh after a change), a list of the entities that changed in the cycle, or a publication by copy of the previous
  slot's whole block.
- The component dispatch is a large share by reading (a lock and a copy of the component list per entity and per
  cycle): measure the share of each part before choosing.
- An optimization order: the three plans apply (`docs/plans/README.md`) — proof by an A/B of the logic cycle.

## ⚠️ Traps

- The publication is what makes a frame consistent (scenes doc 12): skipping an entity must never leave a slot with a
  state older than the one the logic thread last decided.
- A node's world coordinates depend on its parents: "this node did not move" does not mean its world frame is the same.

## References

- Scenes doc 12 § *Plain state shared by the logic and rendering threads* (the measurement, with the render extent).
