---
id: physics-octree-drops-an-entity-outside-the-boundary
title: The physics octree silently drops an entity moved entirely outside the scene boundary
status: open
priority: high
scope: Scenes/Scene.entities.cpp (checkEntityLocationInOctrees), Scenes/OctreeSector.hpp (updateOrInsert)
opened: 2026-10-31
tags: [physics, octree, defect, silent-failure]
---

# The physics octree silently drops an entity moved entirely outside the scene boundary

## Why

`Scene::checkEntityLocationInOctrees()` re-files a collidable entity in the physics octree with `updateOrInsert()`,
which erases it then inserts it again; `OctreeSector::insert()` refuses an element whose volume is entirely outside the
root's bounds, and the result is ignored. The entity is then in NO sector, and the physics step (which gathers its
bodies from the octree) never steps it again — no gravity, no collision, no character control. The rendering octree
handles the same case with an overflow set (`m_renderingOctreeOverflow`); the physics one has none.

Met through the kinematic characters, which had no boundary (fixed 2026-10-31: `Scene::insideBoundaries()`, physics
doc 06). The dynamic bodies are clipped by the boundary step. What remains reachable: a teleport or a script that sets a
position entirely outside the box (`Act.setPosition`, `setNodePosition`, a game's own move).

## What remains

- [ ] Owner decision on the contract: clamp every collidable entity into the box when it is re-filed; or keep an
  outside entity at the root of the physics octree (as `update()` documents: "kept at the root, and filed again when it
  comes back") and let the step gather it; or refuse the move at the trust boundary (console, scripts) with a trace.
- [ ] At least: never silent — a failed physics re-file is traced with the entity's name and position.
- [ ] Prove: a node teleported outside on `game-logic`, then the chosen behaviour, 0 frozen entity.

## References

- Physics doc 06 § The world boundaries hold the kinematic characters too.
