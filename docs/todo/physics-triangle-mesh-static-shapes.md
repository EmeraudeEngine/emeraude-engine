---
id: physics-triangle-mesh-static-shapes
title: Triangle meshes as static collision shapes
status: blocked
priority: unranked
scope: Physics (collision models), Scenes (StaticEntity)
opened: 2026-10-01
blocked-by: [contact-manifold-generation, physics-unified-contact-pipeline]
tags: [physics, collisions, physics-overhaul]
---

# Triangle meshes as static collision shapes

## Why

Phase P5 of the physics overhaul (`docs/physics-overhaul.md`). A static entity collides as a point, a sphere, an AABB or
a capsule. A building, a staircase or a ramp authored as a mesh must be approximated by boxes (citadel's 34 steps are
34 static AABBs). emeraude-base already has triangle tests (tri ↔ AABB SAT, tri ↔ sphere, tri ↔ capsule).

## What remains

- [ ] A static triangle-mesh collision model with its own bounding-volume hierarchy, built once at load.
- [ ] Contacts with the dynamic primitives through `contact-manifold-generation`; internal-edge handling so a body
  sliding across two coplanar triangles does not bump on the shared edge.
- [ ] Shape casts against it for the character controller.

## References

- C. Ericson, *Real-Time Collision Detection* (2005), ch. 6 (BVH); Box2D v3 / Bullet internal-edge handling.
