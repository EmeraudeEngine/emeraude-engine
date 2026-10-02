---
id: ground-kicks-fast-rolling-ball
title: A ball rolling fast on the flat ground is kicked upward at some points (ghost contacts?)
status: open
priority: unranked
scope: Scenes/Scene.physics.cpp (ground contacts, continuous pass), Physics/NarrowPhase
opened: 2026-10-02
tags: [physics, physics-overhaul, ground]
---

# A ball rolling fast on the flat ground is kicked upward at some points (ghost contacts?)

## Why

Seen on `collision-debug`'s `BenchMeshBullet` (P5, 2026-10-02): after its bounce off the mesh wall, the 0.5 m ball rolls
back on the FLAT pavement ground at −37 m/s along z = 26, and is kicked upward three times where nothing stands — cycle
26 at x 61.6 (vy −0.08 → +5.96, vx −37 → −29.9), cycle 102 at x 24.0 (vy +4.69), cycle 161 at x 4.5 — and drifts in z
(26 → 29.8). `BenchBulletBall`, rolling back at the same speed along z = −91 after its bounce off the box wall, is never
kicked. Deterministic (the same cycles in every run). Not the mesh: the wall hit itself is clean (stopped at x 74.495).

## What remains

- [ ] Find the source: a speculative contact with the ground triangle AHEAD whose closest feature is its near edge (a
  leaning normal: the classic internal-edge ghost contact; the ground has no active-edge flags), or the continuous pass
  (step 4b) meeting a ground edge with a leaning normal, or the ground recovery (1b). Record the manifolds of those
  cycles (which triangles, which normals).
- [ ] If it is the edges: give the ground's contacts the triangle mesh's correction (`Space3D::TriangleMesh::
  correctInternalEdgeNormal()`), the ground's edges inactive where flat — decide with the owner what a convex ridge does.

## References

- Engine `docs/subsystems/physics/18-triangle-mesh-statics.md` (the active edges), `docs/physics-overhaul.md` § 1b (P5).
