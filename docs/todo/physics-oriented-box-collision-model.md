---
id: physics-oriented-box-collision-model
title: Rotated boxes collide as boxes, not as their world AABB
status: blocked
priority: unranked
scope: Physics (AABBCollisionModel, CollisionModelInterface, a new oriented box model)
opened: 2026-10-01
blocked-by: [contact-manifold-generation, physics-unified-contact-pipeline]
tags: [physics, collisions, physics-overhaul]
---

# Rotated boxes collide as boxes, not as their world AABB

## Why

Phase P3 of the physics overhaul (`docs/physics-overhaul.md` § 2 B). `AABBCollisionModel::collideWithAABB()` tests the
world AABB of each box (`toWorldAABB()` rebuilds an axis-aligned envelope from the 8 rotated corners). A 45° cube is
1.41× wider than itself for its neighbours; a tipping cube lands on an envelope that grows as it turns. This is what
`collision-debug`'s rotated cubes and its dynamic tipping test show.

## What remains

- [ ] Decide with the owner: a new `OrientedBoxCollisionModel`, or `AABBCollisionModel` becoming oriented (its local
  box follows the entity's rotation) with the axis-aligned test kept as the fast path when both frames are unrotated.
  The auto-created model of an entity (`AbstractEntity::updateEntityProperties()`) follows that decision.
- [ ] Box ↔ box, box ↔ sphere, box ↔ capsule, box ↔ ground through `contact-manifold-generation`.
- [ ] The broad phase keeps using the world AABB (octree), only the narrow phase changes.
- [ ] `collision-debug`: the rotated static and node cubes touch exactly at their faces; the tipping cube rests flat
  on a face of the green cube or on the ground.

## References

- `src/Physics/AABBCollisionModel.cpp`, emeraude-base `src/Math/OrientedCuboid.hpp`.
- `docs/subsystems/physics/07-critical-aabb-world-transform-with-orientedcuboid.md`.
