---
id: mesh-convex-edge-contact-normal
title: A box against a triangle mesh's CONVEX edge gets a facet normal (internal-edge correction), oblique to the impact
status: open
priority: high
scope: Physics/NarrowPhase (generateMeshTriangle), Physics/TriangleMesh (correctInternalEdgeNormal)
opened: 2026-10-28
tags: [physics, collision, defect]
---

# A box against a triangle mesh's CONVEX edge gets a facet normal (internal-edge correction), oblique to the impact

## Why

Found 2026-10-28 by a downstream demo (a car into an 8-sided lamp post, part of a static triangle mesh): every contact
of the car's chassis box with the post carries a FACET normal — (±0.92, 0, 0.38) or (±0.38, 0, 0.92) — while the car
moves along −Z and its front face meets the post head-on. `NarrowPhase::generateMeshTriangle()` →
`TriangleMesh::correctInternalEdgeNormal()`: the internal-edge correction (made so a body sliding over a ground's
SHARED edges feels no bump) also forces a facet normal at the CONVEX edges of a thin shape, where the true contact
normal lies in the edge's normal cone (here the box's face, −Z). The solver then pushes along the facet normals; the
lateral parts cancel when both sides are touched, but a single oblique facet pushes the body sideways. The impact
EVENT was fixed separately (measured along the pair's resultant impulse — physics docs § step 5); the contact itself
is this item.

## What remains

- At a convex mesh edge, accept the contact's own normal when it lies inside the cone of the two adjacent face normals
  (Jolt's active-edge test: `ActiveEdges::FixNormal`, Bullet's `btAdjustInternalEdgeContacts`), correct it only when it
  points outside (a concave / coplanar edge, the ground case it was made for).
- A physics bench by cycle (`tools/physics-bench.py --compare`, the mesh stations: BenchMeshSlider, BenchBoxSlider,
  BenchMeshBall…) before / after: the ground sliding must not regress.

## References

- Jolt Physics, `ActiveEdges.h` (MIT); Bullet, `btInternalEdgeUtility.cpp` (zlib).
